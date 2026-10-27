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

#ifndef NOISE_SOURCE_HPP
#define NOISE_SOURCE_HPP

#include "dsp_component.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <random>

namespace noteahead {

//! Coloured noise with a continuous colour control and an optional sample-and-hold.
//!
//! Colour runs from brown (0) through pink (1/3) and white (2/3) to blue (1). Brown is a leaky
//! integrator, pink is Kellet's refined filter, and blue is the first difference of pink. Every
//! colour is scaled to the RMS of the white noise it is filtered from, and the crossfade between two
//! neighbouring colours corrects for their correlation, so turning the colour knob changes the
//! spectrum without changing the level. The gains are worked out from the filters' impulse responses
//! whenever the sample rate changes.
//!
//! The sample-and-hold holds the coloured output at a rate below the sample rate: "digital" noise.
//!
//! The sequence is fixed by the seed and restarted by reset(), so a render is reproducible.
class NoiseSource : public DspComponent
{
public:
    explicit NoiseSource(uint32_t seed = 0);

    void setSampleRate(double sampleRate) override;

    //! 0 = brown, 1/3 = pink, 2/3 = white, 1 = blue, with crossfades in between.
    void setColour(double colour);
    double colour() const;

    //! Rate at which the output is held, in Hz. Without one, the default, or at or above the sample
    //! rate, every sample is passed through.
    void setHoldRate(std::optional<double> holdRate);
    std::optional<double> holdRate() const;

    double nextSample();

    //! Restarts the random sequence from the seed and clears the filters.
    void reset();

private:
    void updateCoefficients();
    void updateCrossfade();
    double nextColourSample();

    enum Colour
    {
        Brown,
        Pink,
        White,
        Blue,
        ColourCount
    };

    uint32_t m_seed;
    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_dist { -1.0, 1.0 };

    double m_colour { 2.0 / 3.0 };
    std::optional<double> m_holdRate;

    // Brown: leaky integrator
    double m_brownPole { 0.0 };
    double m_brownState { 0.0 };

    // Pink: Kellet's refined filter, poles retuned for the sample rate
    static constexpr int PinkPoleCount { 6 };
    std::array<double, PinkPoleCount> m_pinkPoles {};
    std::array<double, PinkPoleCount> m_pinkInputGains {};
    std::array<double, PinkPoleCount> m_pinkStates {};
    //! Kellet's last tap, which reaches the output one sample late.
    double m_pinkDelayed { 0.0 };
    double m_pinkPrevious { 0.0 };

    //! Scales each colour to the RMS of the white noise.
    std::array<double, ColourCount> m_gains { 1.0, 1.0, 1.0, 1.0 };
    //! Normalized correlation between each colour and the next one up.
    std::array<double, ColourCount - 1> m_correlations { 0.0, 0.0, 0.0 };

    int m_lowerColour { White };
    double m_lowerGain { 1.0 };
    double m_upperGain { 0.0 };

    double m_holdPhase { 0.0 };
    double m_heldValue { 0.0 };
};

//! Sparse random impulses: vinyl crackle, rain, fire.
//!
//! Each sample fires with a probability of density / sample rate. The amplitude is cubed from a
//! uniform draw, so most impulses are small and a few are loud, and the sign is random.
class CrackleSource : public DspComponent
{
public:
    explicit CrackleSource(uint32_t seed = 0);

    //! Average number of impulses per second.
    void setDensity(double density);
    double density() const;

    double nextSample();

    void reset();

private:
    uint32_t m_seed;
    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_dist { 0.0, 1.0 };
    double m_density { 0.0 };
};

} // namespace noteahead

#endif // NOISE_SOURCE_HPP
