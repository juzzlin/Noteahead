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

#include "synth_d_voice.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! What one VCO at full volume puts into the ladder. Its transistors start to round off at about 1,
//! so one VCO is clean, two begin to overdrive and three drive it hard: the Minimoog's mixer trick.
constexpr double MixerLevel { 0.7 };
//! The circuit's own hiss, about -120 dB: what starts the filter singing with every source off
constexpr double NoiseFloor { 1.0e-6 };
//! The overload LED lights where the ladder starts to round the mix off
constexpr double OverloadLevel { 1.0 };
//! Ext In at full: more than the output, so the loop can grow until the ladder holds it
constexpr double MaxFeedbackGain { 1.2 };
//! The output is AC coupled on its way back in, so the loop cannot latch up at DC
constexpr double FeedbackCouplingHz { 15.0 };

//! Pulse widths of the wide and narrow rectangles. Moog never published them; these are what
//! Minimoog owners measure.
constexpr double WidePulseWidth { 0.35 };
constexpr double NarrowPulseWidth { 0.15 };

constexpr double MaxOscModSemitones { 12.0 };
constexpr double MaxFilterModOctaves { 3.0 };

constexpr double MaxDriftCents { 5.0 };
constexpr double MaxInstabilityCents { 3.0 };
constexpr double MaxToleranceCents { 5.0 };
constexpr double MaxToleranceCutoffOctaves { 0.3 };
constexpr size_t ToleranceTune { 0 };
constexpr size_t ToleranceCutoff { 1 };

//! The attack charges towards a level above its peak and stops there, so it rises nearly straight
constexpr double AttackTarget { 1.3 };
//! A decay time is how long the contour takes to fall to 1 % of the way to its sustain level
constexpr double DecayTimeConstants { 4.6 };
//! With the decay switch off the note stops at once: as fast as the contour can without a click
constexpr double ReleaseOffSeconds { 0.002 };
//! The loudness contour's 80 dB range: below this the note is over
constexpr double ContourFloor { 1.0e-4 };

double polyBlep(double t, double dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

double pulse(double phase, double width, double increment)
{
    double value = phase < width ? 1.0 : -1.0;
    value += polyBlep(phase, increment);
    value -= polyBlep(std::fmod(phase + 1.0 - width, 1.0), increment);
    // AC coupled: a narrow pulse sits around zero, not below it
    return value - (2.0 * width - 1.0);
}

double onePoleCoefficient(double seconds, double sampleRate)
{
    return seconds > 0.0 ? 1.0 - std::exp(-1.0 / (seconds * sampleRate)) : 1.0;
}

//! The share a side gets of a source panned to @p position, -1..1: the far side loses it, the near
//! side keeps it at the level it has in the middle, so the filter it drives is driven as hard
double sideGain(double position, size_t side)
{
    return side == 0 ? std::min(1.0, 1.0 - position) : std::min(1.0, 1.0 + position);
}

} // namespace

double SynthDVoice::panelSeconds(double knob, double atZero, double atTen)
{
    // The time knobs are marked 10 ms at a half, 200 ms at 2, 600 ms at 4, 1 s at 6, 5 s at 8 and
    // their longest at 10: steep at first, then half the travel for the last decade and a half.
    // Between the marks the time goes geometrically.
    const std::array<std::pair<double, double>, 7> marks { { { 0.0, atZero }, { 0.05, 0.01 }, { 0.2, 0.2 }, { 0.4, 0.6 }, { 0.6, 1.0 }, { 0.8, 5.0 }, { 1.0, atTen } } };
    const double x = std::clamp(knob, 0.0, 1.0);
    for (size_t i = 1; i < marks.size(); i++) {
        if (x <= marks[i].first) {
            const auto [x0, t0] = marks[i - 1];
            const auto [x1, t1] = marks[i];
            return t0 * std::pow(t1 / t0, (x - x0) / (x1 - x0));
        }
    }
    return atTen;
}

double SynthDVoice::attackSeconds(double knob)
{
    // 1 ms to 10 s
    return panelSeconds(knob, 0.001, 10.0);
}

double SynthDVoice::decaySeconds(double knob)
{
    // 4 ms to beyond 35 s
    return panelSeconds(knob, 0.004, 35.0);
}

double SynthDVoice::glideSeconds(double knob)
{
    // 1 ms to 10 s an octave
    return 0.001 * std::pow(10.0, 4.0 * std::clamp(knob, 0.0, 1.0));
}

double SynthDVoice::lfoHz(double knob)
{
    // 0.05 Hz to 200 Hz
    return 0.05 * std::pow(4000.0, std::clamp(knob, 0.0, 1.0));
}

double SynthDVoice::cutoffHz(double knob)
{
    return LowestCutoffHz * std::pow(HighestCutoffHz / LowestCutoffHz, std::clamp(knob, 0.0, 1.0));
}

double SynthDVoice::oscillatorHz(Range range, double note)
{
    double octaves = 0.0;
    switch (range) {
    case Range::Lo:
        octaves = -LoRangeOctaves;
        break;
    case Range::Feet32:
        octaves = -2.0;
        break;
    case Range::Feet16:
        octaves = -1.0;
        break;
    case Range::Feet8:
        break;
    case Range::Feet4:
        octaves = 1.0;
        break;
    case Range::Feet2:
        octaves = 2.0;
        break;
    }
    return 440.0 * std::exp2((note - 69.0) / 12.0 + octaves);
}

void SynthDVoice::ContourGenerator::set(const Contour & contour, double sampleRate)
{
    const double attack = attackSeconds(contour.attack);
    // Charging towards AttackTarget, the level reaches 1 after this many time constants
    const double attackConstants = std::log(AttackTarget / (AttackTarget - 1.0));
    m_attackStep = onePoleCoefficient(attack / attackConstants, sampleRate);
    const double decay = decaySeconds(contour.decay) / DecayTimeConstants;
    m_decayCoefficient = onePoleCoefficient(decay, sampleRate);
    m_releaseCoefficient = contour.decaySwitch ? m_decayCoefficient : onePoleCoefficient(ReleaseOffSeconds, sampleRate);
    m_sustain = std::clamp(contour.sustain, 0.0, 1.0);
}

void SynthDVoice::ContourGenerator::trigger()
{
    // From wherever the capacitor is: a retrigger does not empty it first
    m_stage = Stage::Attack;
}

void SynthDVoice::ContourGenerator::release()
{
    if (m_stage != Stage::Idle) {
        m_stage = Stage::Release;
    }
}

double SynthDVoice::ContourGenerator::next()
{
    switch (m_stage) {
    case Stage::Idle:
        break;
    case Stage::Attack:
        m_value += m_attackStep * (AttackTarget - m_value);
        if (m_value >= 1.0) {
            m_value = 1.0;
            m_stage = Stage::Decay;
        }
        break;
    case Stage::Decay:
        m_value += m_decayCoefficient * (m_sustain - m_value);
        break;
    case Stage::Release:
        m_value -= m_releaseCoefficient * m_value;
        if (m_value < ContourFloor) {
            m_value = 0.0;
            m_stage = Stage::Idle;
        }
        break;
    }
    return m_value;
}

double SynthDVoice::ContourGenerator::value() const
{
    return m_value;
}

bool SynthDVoice::ContourGenerator::isIdle() const
{
    return m_stage == Stage::Idle;
}

void SynthDVoice::ContourGenerator::reset()
{
    m_stage = Stage::Idle;
    m_value = 0.0;
}

double SynthDVoice::Vco::next(Waveform waveform, bool reverse, double increment)
{
    const double p = phase;
    phase += increment;
    if (phase >= 1.0) {
        phase -= std::floor(phase);
    }

    const double saw = 2.0 * p - 1.0 - polyBlep(p, increment);
    switch (waveform) {
    case Waveform::Triangle:
        return 1.0 - 4.0 * std::abs(p - 0.5);
    case Waveform::TriangleSaw: {
        if (reverse) {
            return -saw;
        }
        // The shark tooth: half a triangle and half a ramp, a steep rise and a slow one, then the drop
        const double triangle = 1.0 - 4.0 * std::abs(p - 0.5);
        return (2.0 / 3.0) * (triangle + saw);
    }
    case Waveform::Saw:
        return saw;
    case Waveform::Square:
        return pulse(p, 0.5, increment);
    case Waveform::WidePulse:
        return pulse(p, WidePulseWidth, increment);
    case Waveform::NarrowPulse:
        return pulse(p, NarrowPulseWidth, increment);
    }
    return 0.0;
}

SynthDVoice::SynthDVoice()
{
    // So that the coefficients worked out from the settings are there before any are given
    setSettings(m_settings);
}

void SynthDVoice::setSampleRate(double sampleRate)
{
    if (sampleRate <= 0.0 || sampleRate == m_sampleRate) {
        return;
    }
    m_sampleRate = sampleRate;
    for (auto && filter : m_filters) {
        filter.setSampleRate(sampleRate * static_cast<double>(Oversampling));
    }
    m_noise.setSampleRate(sampleRate);
    setSettings(m_settings);
}

void SynthDVoice::setSettings(const Settings & settings)
{
    m_settings = settings;
    m_filterContour.set(settings.filterContour, m_sampleRate);
    m_loudnessContour.set(settings.loudnessContour, m_sampleRate);
    m_glideCoefficient = onePoleCoefficient(glideSeconds(settings.glide), m_sampleRate);
    // Fixed by the settings and the rate, so worked out here rather than once a sample
    m_baseCutoffHz = cutoffHz(settings.cutoff);
    m_lfoHz = lfoHz(settings.lfoRate);
    m_feedbackCoupling = onePoleCoefficient(1.0 / (2.0 * std::numbers::pi * FeedbackCouplingHz), m_sampleRate * static_cast<double>(Oversampling));
    m_overloadDecay = 1.0 - onePoleCoefficient(0.1, m_sampleRate);
    m_noise.setColour(settings.noiseColour == NoiseColour::Pink ? 1.0 / 3.0 : 2.0 / 3.0);
    for (auto && filter : m_filters) {
        filter.setMode(settings.filterMode);
        filter.setResonance(settings.emphasis);
    }
}

void SynthDVoice::play(uint8_t note, double velocity, bool trigger)
{
    m_targetNote = static_cast<double>(note);
    if (!m_gate && m_loudnessContour.isIdle()) {
        // Nothing sounding to glide from
        m_note = m_targetNote;
    }
    m_velocity = velocity;
    if (trigger) {
        m_filterContour.trigger();
        m_loudnessContour.trigger();
    }
    m_gate = true;
}

void SynthDVoice::release()
{
    m_gate = false;
    m_filterContour.release();
    m_loudnessContour.release();
}

void SynthDVoice::setPitchBend(double semitones)
{
    m_pitchBend = semitones;
}

double SynthDVoice::modulationSource1(double filterEnvelope) const
{
    return m_settings.modSource1 == ModSource1::Osc3 ? m_vcoOutputs[2] : filterEnvelope;
}

void SynthDVoice::process(double & left, double & right)
{
    const auto & s = m_settings;
    const double rate = m_sampleRate * static_cast<double>(Oversampling);

    // The control voltages, once a sample
    m_note += m_glideCoefficient * (m_targetNote - m_note);

    advanceLfo(1);
    m_noiseValue = m_noise.nextSample();

    const double filterEnvelope = m_filterContour.next();
    const double loudness = m_loudnessContour.next();

    const double source2 = s.modSource2 == ModSource2::Noise ? m_noiseValue : m_lfoValue;
    const double modulation = ((1.0 - s.modMix) * modulationSource1(filterEnvelope) + s.modMix * source2) * s.modDepth * s.modDepth;

    const double drift = s.drift > 0.0 ? s.drift * MaxDriftCents * m_drift.next(m_sampleRate) / 100.0 : 0.0;
    std::array<double, 3> increments {};
    for (size_t i = 0; i < 3; i++) {
        const auto & oscillator = s.oscillators[i];
        double note = oscillator.frequency + s.tolerance * MaxToleranceCents * AnalogDrift::tolerance(i, ToleranceTune) / 100.0;
        if (s.instability > 0.0) {
            note += s.instability * MaxInstabilityCents * m_vcoDrifts[i].next(m_sampleRate) / 100.0;
        }
        if (i < 2 || s.osc3KeyboardControl) {
            note += m_note + s.tune + m_pitchBend + drift;
            if (s.oscillatorModulation) {
                note += modulation * MaxOscModSemitones;
            }
        } else {
            note += Osc3FixedNote;
        }
        increments[i] = std::min(oscillatorHz(oscillator.range, note) / rate, 0.45);
    }

    const double keyboardControl = (s.keyboardControl1 ? 1.0 / 3.0 : 0.0) + (s.keyboardControl2 ? 2.0 / 3.0 : 0.0);
    double octaves = s.contourAmount * MaxContourOctaves * filterEnvelope
      + keyboardControl * (m_note + m_pitchBend - KeyboardControlReferenceNote) / 12.0
      + s.tolerance * MaxToleranceCutoffOctaves * AnalogDrift::tolerance(0, ToleranceCutoff);
    if (s.filterModulation) {
        octaves += modulation * MaxFilterModOctaves;
    }
    const double cutoff = m_baseCutoffHz * std::exp2(octaves);
    for (auto && filter : m_filters) {
        filter.setCutoff(cutoff);
    }

    const double vca = loudness * m_velocity;
    const double feedbackGain = s.feedbackOn ? s.feedback * s.feedback * MaxFeedbackGain : 0.0;
    const std::array<double, 3> positions { -s.panSpread, s.panSpread, 0.0 };
    const double noise = (s.noiseOn ? s.noise * MixerLevel : NoiseFloor) * m_noiseValue;

    std::array<std::array<float, Oversampling>, 2> highRate {};
    double peak = 0.0;
    for (size_t step = 0; step < Oversampling; step++) {
        for (size_t i = 0; i < 3; i++) {
            const auto & oscillator = s.oscillators[i];
            m_vcoOutputs[i] = m_vcos[i].next(oscillator.waveform, i == 2, increments[i]);
        }
        for (size_t side = 0; side < 2; side++) {
            double mix = noise;
            for (size_t i = 0; i < 3; i++) {
                const auto & oscillator = s.oscillators[i];
                if (oscillator.on) {
                    mix += m_vcoOutputs[i] * oscillator.volume * MixerLevel * sideGain(positions[i], side);
                }
            }
            mix += feedbackGain * m_feedback[side];
            peak = std::max(peak, std::abs(mix));
            const double output = m_filters[side].process(mix) * vca;
            // The output goes back in through a coupling capacitor
            m_feedbackDc[side] += m_feedbackCoupling * (output - m_feedbackDc[side]);
            m_feedback[side] = output - m_feedbackDc[side];
            highRate[side][step] = static_cast<float>(output);
        }
    }

    left += static_cast<double>(m_decimators[0].process(highRate[0].data(), static_cast<uint8_t>(Oversampling)));
    right += static_cast<double>(m_decimators[1].process(highRate[1].data(), static_cast<uint8_t>(Oversampling)));

    // The LED lights at once and fades over a tenth of a second, so a peak is seen
    if (peak > OverloadLevel) {
        m_overload = 1.0;
    } else {
        m_overload *= m_overloadDecay;
    }
}

bool SynthDVoice::isActive() const
{
    return m_gate || !m_loudnessContour.isIdle();
}

double SynthDVoice::overload() const
{
    return m_overload;
}

void SynthDVoice::advanceLfo(uint32_t frames)
{
    m_lfoPhase += static_cast<double>(frames) * m_lfoHz / m_sampleRate;
    m_lfoPhase -= std::floor(m_lfoPhase);
    m_lfoValue = m_settings.lfoWaveform == LfoWaveform::Triangle ? 1.0 - 4.0 * std::abs(m_lfoPhase - 0.5) : (m_lfoPhase < 0.5 ? 1.0 : -1.0);
}

double SynthDVoice::lfoValue() const
{
    return m_lfoValue;
}

void SynthDVoice::reset()
{
    for (size_t i = 0; i < 3; i++) {
        m_vcos[i].phase = 0.0;
        m_vcoOutputs[i] = 0.0;
        m_vcoDrifts[i].initialize(i + 1);
    }
    m_drift.initialize(0);
    m_noise.reset();
    m_noiseValue = 0.0;
    m_lfoPhase = 0.0;
    m_lfoValue = -1.0;
    for (size_t side = 0; side < 2; side++) {
        m_filters[side].reset();
        m_feedback[side] = 0.0;
        m_feedbackDc[side] = 0.0;
        m_decimators[side].reset();
    }
    m_filterContour.reset();
    m_loudnessContour.reset();
    m_note = m_targetNote;
    m_pitchBend = 0.0;
    m_overload = 0.0;
    m_gate = false;
}

} // namespace noteahead
