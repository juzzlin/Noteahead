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

namespace {

// Phase delay in samples of the loop filter y[n] = (1-c)*x[n] + c*x[n-1] at angular
// frequency w. The sounding pitch of a delay loop is set by the phase delay at the
// fundamental, not by the group delay at DC, and the two diverge enough in the top
// octaves to be worth several cents.
double loopFilterPhaseDelay(double c, double w)
{
    if (w < 1e-9) {
        return c;
    }
    return std::atan2(c * std::sin(w), (1.0 - c) + c * std::cos(w)) / w;
}

// Phase delay in samples of one all-pass stage H(z) = (a + z^-1) / (1 + a*z^-1).
double allPassPhaseDelay(double a, double w)
{
    if (w < 1e-9) {
        return (1.0 - a) / (1.0 + a);
    }
    const double sinW = std::sin(w);
    const double cosW = std::cos(w);
    return (std::atan2(sinW, a + cosW) - std::atan2(a * sinW, 1.0 + a * cosW)) / w;
}

} // namespace

double WaveguideString::midiNoteToFreq(uint8_t note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

void WaveguideString::setSampleRate(double sampleRate)
{
    const bool rateChanged = m_sampleRate != sampleRate;

    DspComponent::setSampleRate(sampleRate);
    m_delay.setSampleRate(sampleRate);
    m_dispersion.setSampleRate(sampleRate);
    ensureBuffer();

    if (rateChanged && m_frequency > 0.0) {
        // A note struck before the backend reported its rate was tuned against the
        // default one, so re-derive the loop length instead of leaving it sounding off.
        retune();
    }
}

void WaveguideString::ensureBuffer()
{
    // Sized so that even the bottom of the MIDI range fits, and grown only when a
    // higher rate demands it: shrinking would clear the buffer and cut a ringing
    // string short, while the spare capacity costs nothing to keep.
    const auto required = static_cast<size_t>(std::ceil(m_sampleRate / LowestFrequency)) + 2;
    if (m_delay.capacity() < required) {
        m_delay.setMaxDelay(required);
    }
}

size_t WaveguideString::retune()
{
    const double w = 2.0 * std::numbers::pi * m_frequency / m_sampleRate;

    // Everything in the loop contributes to the period, so the delay line only has to
    // supply what the filters do not. The interpolating read contributes its own
    // fractional part to within a thousandth of a sample, which is far below audibility.
    const double loopFilterDelay = loopFilterPhaseDelay(m_loopFilterCoeff, w);
    const double apDelay = ApStages * allPassPhaseDelay(m_dispersionCoeff, w);
    m_delay.setFractionalDelay(m_sampleRate / m_frequency - loopFilterDelay - apDelay);

    return m_delay.delay();
}

void WaveguideString::trigger(uint8_t note, float velocity, float brightness, float inharmonicity, float decayTime, double detuneCents)
{
    ensureBuffer();

    m_frequency = midiNoteToFreq(note) * std::exp2(detuneCents / 1200.0);
    const double freq = m_frequency;

    // Compute the filter parameters first so the delay length can be corrected for
    // the delay they add to the loop. A real string always loses more of its high
    // partials than its low ones on every pass, so the loop filter is never allowed to
    // disappear altogether however bright the tone is asked to be.
    m_loopFilterCoeff = std::max(MinLoopFilterCoeff, static_cast<double>(1.0f - brightness) * 0.5);
    m_loopFilterPrev = 0.0;

    m_dispersionCoeff = static_cast<double>(inharmonicity) * 0.15;
    m_dispersion.setStages(ApStages);
    m_dispersion.setCoefficient(m_dispersionCoeff);
    m_dispersion.reset();

    // The loop length is fractional, so the sounding pitch matches the target exactly
    // instead of snapping to the nearest whole sample. N below is only the integer part,
    // used to size the excitation.
    const size_t N = retune();

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
    // The noise is low-pass filtered to ~16× the fundamental so that high-brightness settings
    // don't leave wideband noise circulating in the delay line indefinitely.
    // The note is mixed into the seed because this generator's first output is nothing but
    // the seed times its multiplier: seeding straight from the note number left every pitch
    // starting its burst on the same near-full-scale negative sample.
    std::minstd_rand rng { static_cast<uint32_t>(note) * 2654435761u + 17u };
    std::uniform_real_distribution<double> noiseDist { -1.0, 1.0 };
    const double noiseGain = static_cast<double>(velocity) * 0.08;
    const double noiseDecay = 6.0 / static_cast<double>(N);
    const double noiseCutoff = std::min(freq * 16.0, m_sampleRate * 0.45);
    const double noiseAlpha = std::exp(-2.0 * std::numbers::pi * noiseCutoff / m_sampleRate);
    double noiseLpState = 0.0;

    // Real hammers never strike in sample-lock, but every voice used to load its pulse at
    // the same point in its delay line, so the notes of a chord all fired on the same
    // sample. The pulse is one-sided, so those peaks added up instead of partly
    // cancelling, and a hard-struck chord led with a spike some 22 dB above its own body.
    // Holding the strike back by a fraction of a period leaves a single note exactly as it
    // was and lets a chord's pulses fall apart. Stepping by the golden ratio puts
    // neighbouring notes — the ones most likely to be struck together — at opposite ends of
    // the spread, which drawing at random would leave to chance.
    const size_t maxStrikeOffset = std::min(N - 1, static_cast<size_t>(m_sampleRate * StrikeSpreadSeconds));
    const double strikePhase = std::fmod(static_cast<double>(note) * std::numbers::phi, 1.0);
    const size_t strikeOffset = static_cast<size_t>(strikePhase * static_cast<double>(maxStrikeOffset));

    // Pre-load excitation into the delay buffer so output starts immediately.
    m_delay.reset();
    for (size_t i = 0; i < strikeOffset; i++) {
        m_delay.write(0.0);
    }
    for (size_t i = 0; i < N - strikeOffset; i++) {
        double excite = 0.0;
        if (i < width) {
            const double t = static_cast<double>(i) / static_cast<double>(width);
            excite = amplitude * 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * t));
        }
        const double rawNoise = noiseDist(rng) * noiseGain * std::exp(-noiseDecay * static_cast<double>(i));
        noiseLpState = noiseAlpha * noiseLpState + (1.0 - noiseAlpha) * rawNoise;
        excite += noiseLpState;
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
    m_frequency = 0.0;
    m_loopGain = 0.0;
    m_loopFilterCoeff = 0.25;
    m_loopFilterPrev = 0.0;
    m_dispersionCoeff = 0.0;
    m_damperGain = 1.0;
    m_damperDecay = 1.0;
    m_releasing = false;
    m_energy = 0.0;
}

} // namespace noteahead
