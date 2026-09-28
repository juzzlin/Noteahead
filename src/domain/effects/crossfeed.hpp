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

#ifndef CROSSFEED_HPP
#define CROSSFEED_HPP

#include "../dsp/delay_line.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "effect.hpp"

namespace noteahead {

//! Headphone crossfeed: each ear also hears the other channel, late and dark.
//!
//! On speakers both ears hear both boxes. The far one arrives a fraction of a millisecond later and
//! duller, because the head is in the way, and that difference is what the ear reads as a direction.
//! Headphones deliver each channel to one ear alone, so a hard-panned part has no far arrival at all
//! and lands inside the head rather than out in front of it.
//!
//! Worked on the side signal rather than by adding each channel to the other: what one ear loses,
//! the other gains, so centred material comes through at exactly the level it went in at. Adding a
//! filtered copy of each channel to the other would instead lift everything already in the middle by
//! up to 3 dB, which is a bass boost dressed up as an image.
//!
//! Monitoring only, like Monitor: an offline render passes through untouched, so a mix can be
//! checked this way without the check reaching the file.
class Crossfeed : public Effect
{
public:
    Crossfeed();

    static std::string typeIdString();

    std::string type() const override;
    std::string typeId() const override;

    void sync() override;
    void reset() override;

    const EffectPresetList & factoryPresets() const override;

protected:
    void processSample(double & left, double & right) override;

    //! Where the export is kept clean. Crossfeed is a property of the listening, so a block being
    //! written to a file passes through whatever this is set to.
    void processBlock(AudioContext & context) override;

private:
    void syncParameters();
    void updateState();

    float m_amount { 0.6f };
    float m_delayUs { 270.0f };
    float m_cutoffHz { 700.0f };
    float m_outputDb { 0.0f };

    //! One line and one filter, because only the side signal goes through them.
    DelayLine m_delay;
    OnePoleFilter m_filter;

    double m_lastSampleRate { -1.0 };
    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // CROSSFEED_HPP
