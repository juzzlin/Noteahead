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

// Magnitude response of the loop filter y[n] = (1-c)*x[n] + c*x[n-1] at angular
// frequency w. Unity at DC and falling from there, so a loop built around it stays
// stable as long as the gain multiplying it does not exceed one.
double loopFilterMagnitude(double c, double w)
{
    return std::hypot((1.0 - c) + c * std::cos(w), c * std::sin(w));
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

// Coefficient of the all-pass stage that delays w by the wanted number of samples. The
// phase delay falls monotonically as the coefficient rises, so a bisection converges on
// it without needing the inverse in closed form, which the frequency dependence does not
// have. Solving it at the sounding frequency rather than taking the value the low
// frequency approximation gives keeps the top octaves in tune, where the two part company.
double allPassCoefficientForDelay(double delay, double w)
{
    double low = -0.98;
    double high = 0.98;
    for (int i = 0; i < 60; i++) {
        const double mid = 0.5 * (low + high);
        if (allPassPhaseDelay(mid, w) > delay) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return 0.5 * (low + high);
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

    // A loop filter set from brightness alone costs the same share of the signal on every
    // lap wherever the note sits, and since a top octave string travels its loop thousands
    // of times a second that fixed share damps it out of all proportion: the filter alone
    // decided the decay above the fifth octave, leaving nothing for the decay setting to
    // say. Holding it to a share of the loss the wanted decay allows leaves the rest of
    // that budget for the loop gain, so the fundamental keeps the decay it was asked for
    // and the filter is left doing what it is for, which is taking the partials down
    // faster than the fundamental. The ceiling only bites in the top octaves, where there
    // are few partials left below Nyquist to tell apart in any case.
    const double budget = (1.0 - targetGainPerLap()) * FilterLossShare;
    const double lossPerUnit = 1.0 - std::cos(w); // 1 - |H| to first order in the coefficient
    const double ceiling = lossPerUnit > 1e-12 ? budget / lossPerUnit : 1.0;
    m_loopFilterCoeff = std::max(std::min(m_brightnessCoeff, ceiling), MinLoopFilterCoeff);

    // Everything in the loop contributes to the period, so the delay line only has to
    // supply what the filters do not.
    const double loopFilterDelay = loopFilterPhaseDelay(m_loopFilterCoeff, w);
    const double apDelay = ApStages * allPassPhaseDelay(m_dispersionCoeff, w);
    const double remaining = m_sampleRate / m_frequency - loopFilterDelay - apDelay;

    // The fraction of a sample the loop still needs is supplied by an all-pass stage
    // rather than by interpolating between two taps. Interpolation would be a low-pass
    // as well as a delay, and one that costs the same fraction of the signal on every
    // lap: at the top of the keyboard the loop is travelled thousands of times a second,
    // which was enough on its own to silence the top octave in a fraction of a second
    // however the decay was set. An all-pass leaves the magnitude alone at every
    // frequency, so what the string loses is left to the loop filter to decide.
    //
    // The integer part is taken half a sample short so that the all-pass is asked for
    // something in the middle of its comfortable range: a stage asked for almost no
    // delay at all needs a coefficient close to one, which rings for many samples after
    // the strike.
    const double wanted = std::max(remaining, 1.5);
    auto integerDelay = static_cast<size_t>(wanted - 0.5);
    integerDelay = std::max(integerDelay, size_t { 1 });
    const double fractional = wanted - static_cast<double>(integerDelay);

    m_delay.setDelay(integerDelay);
    m_tuning.setStages(1);
    m_tuning.setCoefficient(allPassCoefficientForDelay(fractional, w));

    updateLoopGain();

    return m_delay.delay();
}

// Loop gain that would produce the wanted decay time on its own.
// Each cycle of N samples multiplies amplitude by the gain.
// gain^(T60 * freq) = 0.001 → gain = exp(-6.908 / (T60 * freq))
// T60 scales with sqrt(refFreq/freq) so lower notes sustain much longer than higher ones.
double WaveguideString::targetGainPerLap() const
{
    const double refFreq = 261.63; // C4
    const double baseT60 = 0.5 + m_decayTime * 9.5;
    const double T60 = std::clamp(baseT60 * std::sqrt(refFreq / m_frequency), 0.2, 30.0);
    return std::exp(-6.908 / (T60 * m_frequency));
}

void WaveguideString::updateLoopGain()
{
    if (m_frequency <= 0.0) {
        return;
    }

    const double perLap = targetGainPerLap();

    // The loop filter takes its own bite out of the fundamental on every lap, and the
    // number of laps in a second is the frequency itself, so the same filter costs a top
    // octave note hundreds of times what it costs a bass note over the same stretch of
    // time. Dividing that loss out leaves the decay following the time asked for instead
    // of collapsing towards the treble. Held at unity so the loop cannot grow at DC,
    // where the filter has nothing left to give back.
    const double w = 2.0 * std::numbers::pi * m_frequency / m_sampleRate;
    const double filterLoss = loopFilterMagnitude(m_loopFilterCoeff, w);
    m_loopGain = std::clamp(perLap / std::max(filterLoss, 1e-6), 0.0, MaxLoopGain);
}

void WaveguideString::trigger(uint8_t note, float velocity, float brightness, float inharmonicity, float decayTime, double detuneCents)
{
    ensureBuffer();

    m_frequency = midiNoteToFreq(note) * std::exp2(detuneCents / 1200.0);
    const double freq = m_frequency;

    // Compute the filter parameters first so the delay length can be corrected for
    // the delay they add to the loop. What brightness asks for is only the starting
    // point: retuning caps it against the decay budget for this pitch.
    m_brightnessCoeff = static_cast<double>(1.0f - brightness) * 0.5;
    m_loopFilterPrev = 0.0;

    m_dispersionCoeff = static_cast<double>(inharmonicity) * 0.15;
    m_dispersion.setStages(ApStages);
    m_dispersion.setCoefficient(m_dispersionCoeff);
    m_dispersion.reset();
    m_tuning.reset();

    // The loop length is fractional, so the sounding pitch matches the target exactly
    // instead of snapping to the nearest whole sample. N below is only the integer part,
    // used to size the excitation. Retuning settles the loop gain too, since how much the
    // loop filter costs per lap depends on where the note sits.
    m_decayTime = static_cast<double>(decayTime);
    const size_t N = retune();

    // Excitation: raised cosine pulse; width ∝ 1/velocity (harder strike = narrower).
    // The width the strike asks for is a fraction of the loop, which the top of the
    // keyboard cannot supply: rounded down to whole samples it reached one sample by the
    // seventh octave, and a raised cosine one sample wide is nothing but its own zero
    // crossing, so the top octave was left with no hammer at all and spoke only from the
    // felt noise. Below the floor the pulse is widened and its amplitude taken down to
    // match, which keeps the energy the strike puts into the loop — and so the level the
    // note speaks at — the same as the fraction of the loop originally asked for.
    const double nominalWidth = static_cast<double>(N) / (2.0 + static_cast<double>(velocity) * 6.0);
    const size_t width = std::max(MinExcitationWidth, static_cast<size_t>(std::lround(nominalWidth)));
    const double amplitude = static_cast<double>(velocity) * 0.5 * std::sqrt(nominalWidth / static_cast<double>(width));

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

    // Build the strike a lap at a time and let it wrap, so that holding it back can only
    // move it around the loop and never cut it short. Written straight into the line, a
    // pulse starting late enough ran out of room before it finished, which cost the note
    // an arbitrary part of its strike depending on where the offset happened to land.
    // The half sample in the raised cosine keeps a pulse at the floor width from opening
    // on its own zero crossing.
    m_excitation.assign(N, 0.0);
    for (size_t i = 0; i < width; i++) {
        const double t = (static_cast<double>(i) + 0.5) / static_cast<double>(width);
        m_excitation[(strikeOffset + i) % N] += amplitude * 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * t));
    }
    for (size_t i = 0; i < N; i++) {
        const double rawNoise = noiseDist(rng) * noiseGain * std::exp(-noiseDecay * static_cast<double>(i));
        noiseLpState = noiseAlpha * noiseLpState + (1.0 - noiseAlpha) * rawNoise;
        m_excitation[(strikeOffset + i) % N] += noiseLpState;
    }

    // Pre-load excitation into the delay buffer so output starts immediately.
    m_delay.reset();
    for (size_t i = 0; i < N; i++) {
        m_delay.write(m_excitation[i]);
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

    // Fractional tuning all-pass: supplies the part of a sample the integer delay cannot
    const double tuned = m_tuning.process(out);

    // Dispersion all-pass chain (string stiffness / inharmonicity)
    const double dispersed = m_dispersion.process(tuned);

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
    m_tuning.reset();
    m_frequency = 0.0;
    m_loopGain = 0.0;
    m_brightnessCoeff = 0.25;
    m_loopFilterCoeff = 0.25;
    m_loopFilterPrev = 0.0;
    m_dispersionCoeff = 0.0;
    m_damperGain = 1.0;
    m_damperDecay = 1.0;
    m_releasing = false;
    m_energy = 0.0;
}

} // namespace noteahead
