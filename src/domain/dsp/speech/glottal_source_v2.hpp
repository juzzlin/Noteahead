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

#ifndef GLOTTAL_SOURCE_V2_HPP
#define GLOTTAL_SOURCE_V2_HPP

#include "../dsp_component.hpp"

#include <random>

namespace noteahead {

//! Liljencrants-Fant (LF) glottal flow source with exponential return phase.
//!
//! Unlike the piecewise Rosenberg pulse whose sharp closing corner has zero return time and aliases
//! at high fundamental frequencies, the LF pulse adds a smooth return phase (Ra = Ta / T0). Larger
//! Ra values steepen high-frequency spectral rolloff independently of open quotient, while tying a
//! pitch-adaptive minimum Ta to the sample period keeps high-pitched singing clean and free of
//! harsh aliasing.
class GlottalSourceV2 : public DspComponent
{
public:
    GlottalSourceV2();

    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);

    //! Share of the period from opening to peak closing slope (te), clamped 0.25..0.85.
    void setOpenQuotient(double openQuotient);

    //! Asymmetry ratio tp / (te - tp), clamped 1.2..5.0. Higher values tilt the pulse and add upper harmonics.
    void setSpeedQuotient(double speedQuotient);

    //! Normalized return-phase duration (Ra = Ta / T0), clamped 0.002..0.12. Larger values soften upper harmonics.
    void setReturnPhase(double returnPhase);

    void setJitter(double jitter);
    void setShimmer(double shimmer);
    void setSubharmonic(double subharmonic);
    void setSeed(unsigned int seed);
    void setInitialPhase(double phase);

    double nextSample();
    double openness() const;

    void reset();

private:
    void beginPeriod();
    void updatePhaseStep();
    void updateLfParametersIfNeeded();
    void updateLfParameters();
    void updateFrequencyScale();
    double lfFlow(double phase) const;

    static constexpr double MinOpenQuotient { 0.25 };
    static constexpr double MaxOpenQuotient { 0.85 };
    static constexpr double MinSpeedQuotient { 1.2 };
    static constexpr double MaxSpeedQuotient { 5.0 };
    static constexpr double MinReturnPhase { 0.002 };
    static constexpr double MaxReturnPhase { 0.12 };
    static constexpr double MaxReturnFraction { 0.45 };
    static constexpr double AntiAliasingTaFactor { 1.2 };

    double m_frequency { 110.0 };
    double m_openQuotient { 0.5 };
    double m_speedQuotient { 2.5 };
    double m_returnPhase { 0.010 };
    double m_effectiveTa { 0.0 };
    bool m_lfDirty { true };

    double m_te { 0.5 };
    double m_tp { 0.35 };
    double m_wg { 9.0 };
    double m_alpha { 0.0 };
    double m_epsilon { 100.0 };
    double m_expNegEpsTr { 0.0 };
    double m_returnNorm { 1.0 };
    double m_invOpenDenom { 1.0 };
    double m_wgExpNegAlphaTe { 1.0 };
    double m_meanFlow { 1.0 };

    double m_jitter { 0.0 };
    double m_shimmer { 0.0 };
    double m_subharmonic { 0.0 };
    bool m_weakPeriod { false };

    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
    double m_periodScale { 1.0 };
    double m_amplitude { 1.0 };
    double m_openness { 1.0 };

    double m_frequencyScale { 1.0 };
    double m_frequencyScaleTarget { 1.0 };
    double m_frequencyScaleCoefficient { 1.0 };
    bool m_started { false };

    unsigned int m_seed { 0x51EEC4u };
    double m_initialPhase { 0.0 };
    std::mt19937 m_rng { m_seed };
    std::uniform_real_distribution<double> m_deviation { -1.0, 1.0 };
};

} // namespace noteahead

#endif // GLOTTAL_SOURCE_V2_HPP
