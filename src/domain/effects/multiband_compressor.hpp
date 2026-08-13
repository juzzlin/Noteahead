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

#ifndef MULTIBAND_COMPRESSOR_HPP
#define MULTIBAND_COMPRESSOR_HPP

#include "../dsp/compressor_core.hpp"
#include "../dsp/linkwitz_riley_crossover.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

//! Three-band compressor: the signal is split by two Linkwitz-Riley crossovers, each band is
//! compressed on its own terms, and the bands are summed back.
//!
//! Deliberately has no Mix control. The split path is phase-rotated against its own input by the
//! crossovers, so blending it with an untouched dry signal would comb filter rather than soften the
//! compression. Per-band makeup and the output gain cover what Mix would have been reached for.
class MultibandCompressor : public Effect
{
public:
    MultibandCompressor();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;

    static constexpr size_t NumBands = 3;
    static constexpr size_t NumCrossovers = NumBands - 1;

    //! Gain currently applied to a band, in dB, for metering.
    float bandReductionDb(size_t bandIndex) const;

private:
    using BandFrame = std::array<double, NumBands>;

    //! One channel's worth of splitting. Three bands take two crossovers plus a third parked on the
    //! upper corner in its all-pass role, which is what keeps the low band in phase with the two
    //! that were split off after it.
    struct Splitter
    {
        LinkwitzRileyCrossover lowSplit;
        LinkwitzRileyCrossover highSplit;
        LinkwitzRileyCrossover lowAllPass;

        void setCutoffs(double lowerFrequency, double upperFrequency, double sampleRate);
        void split(double input, BandFrame & bands);
        void reset();
    };

    struct Band
    {
        CompressorCore core;
        double makeupDb { 0.0 };
        bool bypassed { false };
        bool soloed { false };
    };

    void updateState();
    void syncParameters();
    void processFrame(double & left, double & right, bool hasSidechain, double sidechainLeft, double sidechainRight);

    std::array<Band, NumBands> m_bands;

    Splitter m_splitterL;
    Splitter m_splitterR;
    Splitter m_sidechainSplitterL;
    Splitter m_sidechainSplitterR;

    std::array<double, NumCrossovers> m_crossoverFrequencies { 200.0, 2000.0 };
    double m_outputGain { 1.0 };
    bool m_anyBandSoloed { false };
    std::optional<size_t> m_sidechainSourceDevice;

    bool m_shouldSyncParameters { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // MULTIBAND_COMPRESSOR_HPP
