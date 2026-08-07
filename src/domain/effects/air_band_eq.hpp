// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef AIR_BAND_EQ_HPP
#define AIR_BAND_EQ_HPP

#include "../dsp/one_pole_filter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace noteahead {

//! Six-band "air band" equalizer in the tradition of the classic 500-series program EQ.
//!
//! What separates this from an ordinary fixed-frequency EQ is the topology. The bands are not
//! cascaded shaping filters; they are parallel taps, each fed the dry signal and summed back into
//! an untouched dry path. Everything the design is known for follows from that one decision:
//!
//! - The bands interact. Overlapping taps add rather than compound, so lowering all five band
//!   passes by the same amount drops the whole curve without reshaping it. That is precisely the
//!   move the hardware documentation prescribes for compensating the air band's added gain.
//! - The usable gain range is asymmetric: +15 dB up against only -4.5 dB down. A tap can add
//!   without limit, but it can never subtract more than the dry signal it was derived from, so cuts
//!   run out of room long before boosts do.
//! - The air band's transition is far gentler than its filter order suggests. Summing a first-order
//!   tap into a dry path gives roughly 2 dB/octave across the audible skirt, so the selected corner
//!   brightens octaves below itself instead of behaving like a conventional shelf. This is why the
//!   40 kHz position audibly does something at ordinary sample rates.
class AirBandEq : public Effect
{
public:
    AirBandEq();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    //! Band passes on the panel: four bells plus the 2.5 kHz shelf. The air band is separate.
    static constexpr size_t BandCount = 5;
    //! The subset of the band passes realised as band-pass taps; the remainder is the shelf.
    static constexpr size_t BellCount = 4;

private:
    //! One channel's worth of taps. Processed as dual mono, matching the single-channel original.
    struct ChannelState
    {
        std::array<SvfFilter, BellCount> bells;
        OnePoleFilter shelf;
        OnePoleFilter air;

        void reset();
    };

    void syncParameters();
    void updateBuffers();
    void processStereo(double & left, double & right);
    double processChannel(double input, ChannelState & state);

    std::array<ChannelState, 2> m_channels;

    //! Coefficients each tap is scaled by before summing into the dry path, not raw gains.
    std::array<double, BandCount> m_bandGains {};
    double m_airGain { 0.0 };
    double m_outputGain { 1.0 };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // AIR_BAND_EQ_HPP
