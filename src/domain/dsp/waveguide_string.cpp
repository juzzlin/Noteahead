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

#include "waveguide_string.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace noteahead {

double WaveguideString::midiNoteToFreq(uint8_t note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

void WaveguideString::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    m_delay.setSampleRate(sampleRate);
    m_dispersion.setSampleRate(sampleRate);
}

void WaveguideString::trigger(uint8_t note, float velocity, float brightness, float inharmonicity, float decayTime, double detuneCents)
{
    const double freq = midiNoteToFreq(note) * std::pow(2.0, detuneCents / 1200.0);

    // Compute filter parameters first so N can be corrected for loop delay.
    // Loop filter FIR y[n]=(1-c)*x[n]+c*x[n-1] has group delay ≈ c samples at DC.
    m_loopFilterCoeff = static_cast<double>(1.0f - brightness) * 0.5;
    m_loopFilterPrev = 0.0;

    // All-pass chain H(z)=(a+z^-1)/(1+a*z^-1): group delay = (1-a)/(1+a) per stage at DC.
    constexpr int ApStages = 4;
    const double apCoeff = static_cast<double>(inharmonicity) * 0.15;
    m_dispersion.setStages(ApStages);
    m_dispersion.setCoefficient(apCoeff);
    m_dispersion.reset();

    // Subtract the combined filter delay from the delay-line length so the sounding
    // pitch matches the target frequency.
    const double loopFilterDelay = m_loopFilterCoeff;
    const double apDelay = ApStages * (1.0 - apCoeff) / (1.0 + apCoeff);
    const size_t N = std::clamp(
      static_cast<size_t>(std::round(m_sampleRate / freq - loopFilterDelay - apDelay)),
      size_t { 4 }, MaxDelaySamples);

    m_delay.setMaxDelay(N);
    m_delay.setDelay(N);

    // Loop gain derived from desired T60 decay time.
    // Each cycle of N samples multiplies amplitude by loopGain.
    // loopGain^(T60 * freq) = 0.001 → loopGain = exp(-6.908 / (T60 * freq))
    // T60 scales with sqrt(refFreq/freq) so lower notes sustain much longer than higher ones.
    const double refFreq = 261.63; // C4
    const double baseT60 = 0.5 + static_cast<double>(decayTime) * 9.5;
    const double T60 = std::clamp(baseT60 * std::sqrt(refFreq / freq), 0.2, 30.0);
    m_loopGain = std::clamp(std::exp(-6.908 / (T60 * freq)), 0.0, 0.99999);

    // Excitation: raised cosine pulse; width ∝ 1/velocity (harder strike = narrower)
    const size_t width = std::max(size_t { 1 }, N / std::max(size_t { 1 }, static_cast<size_t>(2.0 + static_cast<double>(velocity) * 6.0)));
    const double amplitude = static_cast<double>(velocity) * 0.5;

    // Noise burst seeded by note so repeated strikes are consistent but each pitch differs.
    // Decays in ~N/6 samples to model brief hammer-felt impact noise before the tone settles.
    std::minstd_rand rng { static_cast<uint32_t>(note) + 17u };
    std::uniform_real_distribution<double> noiseDist { -1.0, 1.0 };
    const double noiseGain = static_cast<double>(velocity) * 0.08;
    const double noiseDecay = 6.0 / static_cast<double>(N);

    // Pre-load excitation into the delay buffer so output starts immediately.
    m_delay.reset();
    for (size_t i = 0; i < N; i++) {
        double excite = 0.0;
        if (i < width) {
            const double t = static_cast<double>(i) / static_cast<double>(width);
            excite = amplitude * 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * t));
        }
        excite += noiseDist(rng) * noiseGain * std::exp(-noiseDecay * static_cast<double>(i));
        m_delay.write(excite);
    }

    m_damperGain = 1.0;
    m_damperDecay = 1.0;
    m_releasing = false;

    m_energy = 1.0;
}

void WaveguideString::release(float releaseTime)
{
    if (m_releasing)
        return;
    m_releasing = true;
    const double rampSamples = std::max(1.0, static_cast<double>(releaseTime) * m_sampleRate);
    // Decay damperGain by 60 dB over rampSamples
    m_damperDecay = std::pow(0.001, 1.0 / rampSamples);
}

double WaveguideString::nextSample()
{
    if (!isActive())
        return 0.0;

    const double out = m_delay.read();

    // Dispersion all-pass chain (string stiffness / inharmonicity)
    const double dispersed = m_dispersion.process(out);

    // Loop filter: 2-point weighted average for frequency-dependent decay
    const double filtered = (1.0 - m_loopFilterCoeff) * dispersed + m_loopFilterCoeff * m_loopFilterPrev;
    m_loopFilterPrev = dispersed;

    // Damper multiplies loop gain toward zero on note-off
    if (m_releasing) {
        m_damperGain *= m_damperDecay;
    }

    m_delay.write(filtered * m_loopGain * m_damperGain);

    // Exponential moving average of squared output for activity detection
    m_energy = 0.999 * m_energy + 0.001 * out * out;

    return out;
}

bool WaveguideString::isActive() const
{
    return m_energy > SilenceThreshold;
}

void WaveguideString::reset()
{
    m_delay.reset();
    m_dispersion.reset();
    m_loopGain = 0.0;
    m_loopFilterCoeff = 0.25;
    m_loopFilterPrev = 0.0;
    m_damperGain = 1.0;
    m_damperDecay = 1.0;
    m_releasing = false;
    m_energy = 0.0;
}

} // namespace noteahead
