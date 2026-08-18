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

#ifndef STEREO_WIDENER_HPP
#define STEREO_WIDENER_HPP

#include "../dsp/linkwitz_riley_crossover.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

//! Three-band stereo widener: the signal is split by two Linkwitz-Riley crossovers, each band is
//! widened or narrowed on its own terms, and the bands are summed back.
//!
//! A single width control has to be set for whichever part of the spectrum matters most and is
//! wrong everywhere else. Widening that helps the top usually hurts the bottom, because low
//! frequencies carry most of the energy and almost none of the localisation: pushing them apart
//! spends headroom, smears the centre and is the first thing to collapse when the mix is summed to
//! mono. Splitting first is what lets the top be opened up while the bottom is left alone or pulled
//! together.
//!
//! Width works on the side signal, so a band that arrives mono stays mono however far it is turned
//! up: there is nothing off-centre in it to widen. At 0% a band is summed to mono, at 100% it is
//! untouched, and at 200% its side signal is doubled.
//!
//! Mono Bass is a separate stage after the bands, not one of them. It exists to be a guarantee
//! rather than a suggestion -- everything below its corner is centred no matter what the low band's
//! width control did -- and a guarantee has to come last.
//!
//! Deliberately has no Mix control. The split path is phase-rotated against its own input by the
//! crossovers, so blending it with an untouched dry signal would comb filter rather than soften the
//! widening. The output gain covers what Mix would have been reached for.
class StereoWidener : public Effect
{
public:
    StereoWidener();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    static constexpr size_t NumBands = 3;
    static constexpr size_t NumCrossovers = NumBands - 1;

    //! How correlated a band's two channels currently are, from -1 to 1, for the dialog's meters.
    //! Measured after the band's own width control, which is what makes it a reading of what that
    //! control did: 1 is mono, 0 is fully decorrelated, and anything sustained below 0 is the band
    //! being out of phase with itself and will cancel when the mix is summed to mono.
    float bandCorrelation(size_t bandIndex) const;

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

    //! Running correlation of one band, smoothed so that the reading can be followed by eye.
    struct CorrelationMeter
    {
        double productLr { 0.0 };
        double squareL { 0.0 };
        double squareR { 0.0 };

        void update(double left, double right, double coefficient);
        float correlation() const;
        void reset();
    };

    void updateState();
    void syncParameters();

    //! Scales a frame's side signal, leaving its mid alone.
    static void applyWidth(double & left, double & right, double width);

    std::array<double, NumBands> m_bandWidths { 1.0, 1.0, 1.0 };
    std::array<bool, NumBands> m_bandSoloed { false, false, false };
    std::array<CorrelationMeter, NumBands> m_meters;

    Splitter m_splitterL;
    Splitter m_splitterR;

    //! The Mono Bass stage, which needs a crossover of its own because its corner has nothing to do
    //! with where the bands were split.
    LinkwitzRileyCrossover m_monoSplitL;
    LinkwitzRileyCrossover m_monoSplitR;

    std::array<double, NumCrossovers> m_crossoverFrequencies { 250.0, 3000.0 };
    double m_monoFrequency { 120.0 };
    bool m_monoBass { false };
    double m_outputGain { 1.0 };
    bool m_anyBandSoloed { false };
    double m_meterCoefficient { 0.0 };

    bool m_shouldSyncParameters { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // STEREO_WIDENER_HPP
