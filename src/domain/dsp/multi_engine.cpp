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

#include "multi_engine.hpp"
#include "../../common/constants.hpp"
#include "upsampler.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Band-limited saw and square tables for the VPM carriers, one per octave of harmonic count.
//! Phase modulating a band-limited table still aliases once the sidebands pass Nyquist, as all
//! phase modulation does, but the carrier itself stays clean at any pitch.
class CarrierTables
{
public:
    static constexpr size_t TableSize { 2048 };
    static constexpr size_t LevelCount { 11 }; // 1, 2, 4 ... 1024 harmonics

    CarrierTables()
    {
        for (size_t level = 0; level < LevelCount; level++) {
            const size_t harmonics = size_t { 1 } << level;
            auto & saw = m_saw.at(level);
            auto & square = m_square.at(level);
            saw.resize(TableSize + 1);
            square.resize(TableSize + 1);
            for (size_t i = 0; i <= TableSize; i++) {
                const double phase = static_cast<double>(i) / static_cast<double>(TableSize);
                double sawValue = 0.0;
                double squareValue = 0.0;
                for (size_t n = 1; n <= harmonics; n++) {
                    const double partial = std::sin(2.0 * std::numbers::pi * static_cast<double>(n) * phase) / static_cast<double>(n);
                    sawValue -= partial;
                    if (n % 2 == 1) {
                        squareValue += partial;
                    }
                }
                saw.at(i) = static_cast<float>(sawValue);
                square.at(i) = static_cast<float>(squareValue);
            }
            // Each to a peak of one: the ringing of a truncated series overshoots, by more the
            // fewer harmonics there are, and a carrier is not supposed to clip the drive after it.
            normalize(saw);
            normalize(square);
        }
    }

    //! @p harmonics is how many fit under Nyquist at the pitch being played.
    double saw(double phase, size_t harmonics) const
    {
        return lookup(m_saw.at(level(harmonics)), phase);
    }

    double square(double phase, size_t harmonics) const
    {
        return lookup(m_square.at(level(harmonics)), phase);
    }

private:
    using Table = std::vector<float>;

    static void normalize(std::vector<float> & table)
    {
        float peak = 0.0f;
        for (auto && value : table) {
            peak = std::max(peak, std::abs(value));
        }
        for (auto && value : table) {
            value /= peak;
        }
    }

    static size_t level(size_t harmonics)
    {
        size_t level = 0;
        while (level + 1 < LevelCount && (size_t { 1 } << (level + 1)) <= harmonics) {
            level++;
        }
        return level;
    }

    static double lookup(const Table & table, double phase)
    {
        const double position = phase * static_cast<double>(TableSize);
        const auto index = static_cast<size_t>(position);
        const double fraction = position - static_cast<double>(index);
        return table.at(index) + (table.at(index + 1) - table.at(index)) * fraction;
    }

    std::array<Table, LevelCount> m_saw;
    std::array<Table, LevelCount> m_square;
};

//! Built the first time an engine is made, which is never on the audio thread.
const CarrierTables & carrierTables()
{
    static const CarrierTables tables;
    return tables;
}

double wrap(double phase)
{
    return phase - std::floor(phase);
}

//! Middle C, above which Mod Key Track starts taking the depth away.
constexpr double KeyTrackReference = 261.6255653005986;

constexpr double MaxFeedback = 2.0;
constexpr double MaxNoiseDepth = 3.0;
constexpr double MaxEnvelopeDepth = 10.0;
constexpr double MaxDepth = 12.0;
//! How far Mod Attack and Mod Decay stretch or shrink the type's times: 16x either way.
constexpr double EnvelopeTimeOctaves = 4.0;

} // namespace

MultiEngine::MultiEngine()
  : m_dist(-1.0f, 1.0f)
{
    carrierTables();
}

std::vector<std::string> MultiEngine::vpmTypeNames()
{
    return { "Sin1", "Sin2", "Sin3", "Sin4", "Saw1", "Saw2", "Squ1", "Squ2",
             "Fat1", "Fat2", "Air1", "Air2", "Decay1", "Decay2", "Creep", "Throat" };
}

std::vector<double> MultiEngine::vpmRatioOffsets()
{
    return { 0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };
}

std::vector<std::string> MultiEngine::vpmRatioOffsetNames()
{
    return { "1:4", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1" };
}

const std::array<MultiEngine::VpmDefinition, MultiEngine::VpmTypeCount> & MultiEngine::vpmDefinitions()
{
    using Wave = VpmDefinition::Wave;
    using Modulator = VpmDefinition::Modulator;
    // After the XD manual's one-line recipes. Creep and Throat it only calls experimental, atonal
    // and evolving, so those two are ours outright.
    static const std::array<VpmDefinition, VpmTypeCount> definitions { {
      // Sin1: sine carrier and sine modulator.
      { .depthRange = 4.0 },
      // Sin2: the modulator with self-feedback.
      { .depthRange = 4.0, .feedback = 1.0 },
      // Sin3: a modulator on the third harmonic.
      { .ratio = 3.0, .depthRange = 3.0 },
      // Sin4: a modulator on the fifth harmonic.
      { .ratio = 5.0, .depthRange = 2.5 },
      // Saw1: a modulated saw carrier.
      { .carrier = Wave::Saw, .depthRange = 3.0 },
      // Saw2: a sine made into a saw by feeding the carrier back into itself, so a round one.
      { .depthRange = 3.0, .carrierFeedback = 1.3 },
      // Squ1: a modulated square carrier.
      { .carrier = Wave::Square, .depthRange = 3.0 },
      // Squ2: a sine made into a square by a second-harmonic modulator, which leaves only odd
      // harmonics, and a round square at Shape zero.
      { .ratio = 2.0, .baseDepth = 1.2, .depthRange = 3.0 },
      // Fat1: a quarter subharmonic modulator with feedback, and a driven carrier.
      { .ratio = 0.25, .depthRange = 4.0, .feedback = 0.8, .drive = 3.0 },
      // Fat2: a three-quarter subharmonic modulator with more of both.
      { .ratio = 0.75, .depthRange = 4.0, .feedback = 1.4, .drive = 6.0 },
      // Air1: the modulator is noise, so Shape blows wind through a sine.
      { .modulator = Modulator::Noise, .depthRange = 2.0, .noiseCutoff = 3000.0 },
      // Air2: noise and a sine both, the noise slow enough to make the pitch swarm on its own. Shape
      // deepens and opens the noise as well, until the sine is lost in it.
      { .depthRange = 3.0, .noiseDepth = 0.8, .noiseCutoff = 250.0, .noiseShapeRange = 2.2, .noiseShapeOctaves = 3.0 },
      // Decay1: modulation that dies away fast, a spring.
      { .depthRange = 2.0, .feedback = 0.6, .envelopeDepth = 5.0, .attack = 0.001, .decay = 0.35 },
      // Decay2: deep modulation on an inharmonic ratio dying away slowly, a bell settling into a sine.
      { .ratio = 3.5, .depthRange = 2.0, .envelopeDepth = 7.0, .attack = 0.001, .decay = 1.5 },
      // Creep: an atonal ratio whose modulation creeps in and whose ratio never sits still.
      { .ratio = 1.41, .depthRange = 3.0, .feedback = 0.5, .envelopeDepth = 4.0, .attack = 1.2, .decay = 3.0, .ratioWander = 0.02, .ratioWanderRate = 0.17 },
      // Throat: a shallow index on a unity ratio keeps the energy in the lowest harmonics, an "oo".
      // The envelope opens it as the note starts and a slow wobble keeps it talking.
      { .baseDepth = 0.6, .depthRange = 2.0, .feedback = 0.3, .envelopeDepth = 1.2, .attack = 0.25, .decay = 1.5, .depthWobble = 0.35, .depthWobbleRate = 3.0 },
    } };
    return definitions;
}

double MultiEngine::applyOffset(double base, double offset, double maximum)
{
    // The XD's -100..+100 % around the type's own value: below zero scales it down to nothing,
    // above zero takes it up to the maximum, so both ends mean the same thing for every type.
    return offset < 0.0 ? base * (1.0 + offset) : base + offset * (std::max(maximum, base) - base);
}

void MultiEngine::setEngine(Engine engine)
{
    if (m_engine != engine) {
        m_engine = engine;
        reset();
    }
}

MultiEngine::Engine MultiEngine::engine() const
{
    return m_engine;
}

void MultiEngine::setVpmType(VpmType type)
{
    m_vpmType = static_cast<size_t>(type) < VpmTypeCount ? type : VpmType::Sin1;
}

void MultiEngine::setVpmRatioOffset(size_t index)
{
    m_vpmRatioOffset = std::min(index, vpmRatioOffsets().size() - 1);
}

void MultiEngine::setVpmOffsets(const VpmOffsets & offsets)
{
    const auto clampOffset = [](double value) { return std::clamp(value, -1.0, 1.0); };
    m_vpmOffsets = { clampOffset(offsets.feedback), clampOffset(offsets.noiseDepth), clampOffset(offsets.shapeModInt),
                     clampOffset(offsets.modAttack), clampOffset(offsets.modDecay), clampOffset(offsets.modKeyTrack) };
}

void MultiEngine::setFrequency(double frequency)
{
    m_frequency = std::max(0.0, frequency);
}

void MultiEngine::trigger(bool resetPhases)
{
    if (resetPhases) {
        m_carrierPhase = 0.0;
        m_modulatorPhase = 0.0;
        m_modulatorHistory = {};
        m_carrierHistory = {};
        m_envelope = 0.0;
    }
    m_envelopeAttacking = true;
}

void MultiEngine::setType(Type type)
{
    if (m_type != type) {
        m_type = type;
        reset();
    }
}

void MultiEngine::setShape(float shape)
{
    m_shape = std::clamp(shape, 0.0f, 1.0f);
}

void MultiEngine::setKeyTrack(float keyTrack)
{
    m_keyTrack = std::clamp(keyTrack, 0.0f, 1.0f);
}

void MultiEngine::setNote(uint8_t note)
{
    m_note = note;
}

void MultiEngine::reset()
{
    m_rng.seed(RngSeed);
    m_s1 = 0.0;
    m_s2 = 0.0;
    m_phase = 0.0;
    m_lastSample = 0.0f;
    m_lastFrequency = -1.0;
    m_lastResonance = -1.0;
    m_lastDecimRateParam = -1.0f;
    m_lastNote = 0xFF;
    m_carrierPhase = 0.0;
    m_modulatorPhase = 0.0;
    m_wanderPhase = 0.0;
    m_wobblePhase = 0.0;
    m_modulatorHistory = {};
    m_carrierHistory = {};
    m_envelope = 0.0;
    m_envelopeAttacking = false;
    m_phaseNoise = 0.0;
}

void MultiEngine::setOversampleFactor(uint8_t factor)
{
    m_noiseGain = noiseGainForOversampling(factor);
}

float MultiEngine::nextSample()
{
    return m_engine == Engine::Vpm ? nextVpmSample() : nextNoiseSample();
}

double MultiEngine::nextPhaseNoise(double cutoff)
{
    // White noise low passed to the type's corner and brought back to unit RMS, so the depth means
    // the same thing whatever the corner and whatever the oversampling.
    const double a = 1.0 - std::exp(-2.0 * std::numbers::pi * std::min(cutoff, m_sampleRate * 0.45) / m_sampleRate);
    m_phaseNoise += a * (static_cast<double>(m_dist(m_rng)) - m_phaseNoise);
    return m_phaseNoise * std::sqrt(3.0 * (2.0 - a) / a);
}

double MultiEngine::carrierWave(VpmDefinition::Wave wave, double phase) const
{
    switch (wave) {
    case VpmDefinition::Wave::Saw:
    case VpmDefinition::Wave::Square: {
        const auto harmonics = m_frequency > 0.0 ? static_cast<size_t>(m_sampleRate * 0.45 / m_frequency) : CarrierTables::TableSize;
        return wave == VpmDefinition::Wave::Saw ? carrierTables().saw(phase, std::max<size_t>(1, harmonics))
                                                : carrierTables().square(phase, std::max<size_t>(1, harmonics));
    }
    case VpmDefinition::Wave::Sine:
        break;
    }
    return std::sin(2.0 * std::numbers::pi * phase);
}

float MultiEngine::nextVpmSample()
{
    const auto & type = vpmDefinitions().at(static_cast<size_t>(m_vpmType));
    const double sampleRate = m_sampleRate;

    // The internal envelope: a straight attack, then an exponential decay that is 60 dB down at
    // the decay time.
    const double attack = type.attack * std::exp2(m_vpmOffsets.modAttack * EnvelopeTimeOctaves);
    const double decay = type.decay * std::exp2(m_vpmOffsets.modDecay * EnvelopeTimeOctaves);
    if (m_envelopeAttacking) {
        m_envelope += 1.0 / std::max(1.0, attack * sampleRate);
        if (m_envelope >= 1.0) {
            m_envelope = 1.0;
            m_envelopeAttacking = false;
        }
    } else {
        m_envelope *= std::exp(-6.907755278982137 / std::max(1.0, decay * sampleRate));
    }

    double depth = type.baseDepth + static_cast<double>(m_shape) * type.depthRange
      + applyOffset(type.envelopeDepth, m_vpmOffsets.shapeModInt, MaxEnvelopeDepth) * m_envelope;
    if (type.depthWobble > 0.0) {
        m_wobblePhase = wrap(m_wobblePhase + type.depthWobbleRate / sampleRate);
        depth *= 1.0 + type.depthWobble * std::sin(2.0 * std::numbers::pi * m_wobblePhase);
    }
    const double keyTrack = applyOffset(type.keyTrack, m_vpmOffsets.modKeyTrack, 1.0);
    if (m_frequency > KeyTrackReference) {
        depth *= std::pow(KeyTrackReference / m_frequency, keyTrack);
    }
    depth = std::clamp(depth, 0.0, MaxDepth);

    // The modulator
    double ratio = type.ratio * vpmRatioOffsets().at(m_vpmRatioOffset);
    if (type.ratioWander > 0.0) {
        m_wanderPhase = wrap(m_wanderPhase + type.ratioWanderRate / sampleRate);
        ratio *= 1.0 + type.ratioWander * std::sin(2.0 * std::numbers::pi * m_wanderPhase);
    }
    double modulator = 0.0;
    if (type.modulator == VpmDefinition::Modulator::Noise) {
        modulator = nextPhaseNoise(type.noiseCutoff);
    } else {
        // Feedback averages the last two outputs, which keeps a deep setting from flipping
        // between two values every other sample.
        const double feedback = applyOffset(type.feedback, m_vpmOffsets.feedback, MaxFeedback);
        const double fed = feedback * 0.5 * (m_modulatorHistory.at(0) + m_modulatorHistory.at(1));
        modulator = std::sin(2.0 * std::numbers::pi * m_modulatorPhase + fed);
        m_modulatorHistory = { modulator, m_modulatorHistory.at(0) };
        m_modulatorPhase = wrap(m_modulatorPhase + m_frequency * ratio / sampleRate);
    }

    // The carrier, with the modulator, the noise and its own feedback all on its phase
    double phaseOffset = depth * modulator;
    // Squared, so the noise holds back through the middle of the knob and takes over at the top
    const double shape = static_cast<double>(m_shape) * static_cast<double>(m_shape);
    const double noiseDepth = applyOffset(type.noiseDepth + shape * type.noiseShapeRange, m_vpmOffsets.noiseDepth, MaxNoiseDepth);
    if (noiseDepth > 0.0) {
        // A noise modulator already has noise to hand, and a second draw would run its filter twice.
        const double noiseCutoff = type.noiseCutoff * std::exp2(shape * type.noiseShapeOctaves);
        phaseOffset += noiseDepth * (type.modulator == VpmDefinition::Modulator::Noise ? modulator : nextPhaseNoise(noiseCutoff));
    }
    if (type.carrierFeedback > 0.0) {
        phaseOffset += type.carrierFeedback * 0.5 * (m_carrierHistory.at(0) + m_carrierHistory.at(1));
    }
    double carrier = carrierWave(type.carrier, wrap(m_carrierPhase + phaseOffset / (2.0 * std::numbers::pi)));
    m_carrierHistory = { carrier, m_carrierHistory.at(0) };
    m_carrierPhase = wrap(m_carrierPhase + m_frequency / sampleRate);

    if (type.drive > 1.0) {
        carrier = std::tanh(carrier * type.drive) / std::tanh(type.drive);
    }
    return static_cast<float>(carrier);
}

float MultiEngine::nextNoiseSample()
{
    float noise = m_dist(m_rng) * m_noiseGain;

    if (m_type == Type::High) {
        // High-pass [20Hz ... 20kHz]
        const float cutoff = m_shape * m_shape; // Use multiplication instead of std::pow
        updateCoefficients(keyTrackedFrequency(shapeToFrequency(cutoff)), 0.1f);
        return processFilter(noise, 1); // 1 = HP
    } else if (m_type == Type::Low) {
        // Low-pass [20Hz ... 20kHz]
        const float cutoff = m_shape * m_shape;
        updateCoefficients(keyTrackedFrequency(shapeToFrequency(cutoff)), 0.1f);
        return processFilter(noise, 0); // 0 = LP
    } else if (m_type == Type::Peak) {
        // Bandpass [110Hz ... 880Hz]
        const double freqHz = 110.0 + static_cast<double>(m_shape) * (880.0 - 110.0);
        updateCoefficients(keyTrackedFrequency(freqHz), 0.8f);
        return processFilter(noise, 2); // 2 = BP
    } else if (m_type == Type::Decim) {
        // Decimator [240Hz ... 48kHz]
        updateDecimRate();
        m_phase += m_decimRate;
        if (m_phase >= 1.0) {
            m_phase -= 1.0;
            m_lastSample = noise;
        }
        return m_lastSample;
    }

    return noise;
}

double MultiEngine::maxFrequency() const
{
    return std::min(20000.0, m_sampleRate * 0.49);
}

double MultiEngine::shapeToFrequency(float shape) const
{
    return 20.0 * std::exp2(static_cast<double>(shape) * std::log2(maxFrequency() / 20.0));
}

double MultiEngine::keyTrackedFrequency(double frequency) const
{
    return frequency * std::exp2((static_cast<double>(m_note) - 60.0) / 12.0 * static_cast<double>(m_keyTrack));
}

void MultiEngine::updateCoefficients(double frequency, float resonance)
{
    const double clampedFrequency = std::clamp(frequency, 10.0, maxFrequency());

    if (std::abs(clampedFrequency - m_lastFrequency) < 0.000001 && std::abs(resonance - m_lastResonance) < 0.000001f && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    m_g = std::tan(std::numbers::pi * clampedFrequency / m_sampleRate);
    m_k = 2.0 * (1.0 - resonance);
    m_damping = 1.0 / (1.0 + m_g * (m_g + m_k));

    m_lastFrequency = clampedFrequency;
    m_lastResonance = resonance;
    m_lastSampleRate = m_sampleRate;
}

void MultiEngine::updateDecimRate()
{
    if (std::abs(m_shape - m_lastDecimRateParam) < 0.000001f && m_note == m_lastNote && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    float baseRate = 240.0f + m_shape * (static_cast<float>(Constants::defaultSampleRate()) - 240.0f);
    float trackedRate = baseRate * std::exp2((static_cast<float>(m_note) - 60.0f) / 12.0f * m_keyTrack);
    trackedRate = std::clamp(trackedRate, 240.0f, static_cast<float>(m_sampleRate));

    m_decimRate = static_cast<double>(trackedRate) / m_sampleRate;

    m_lastDecimRateParam = m_shape;
    m_lastNote = m_note;
    m_lastSampleRate = m_sampleRate;
}

float MultiEngine::processFilter(float input, int mode)
{
    const double hp = (input - (m_g + m_k) * m_s1 - m_s2) * m_damping;
    const double v1 = m_g * hp;
    const double bp = v1 + m_s1;
    m_s1 = v1 + bp;
    const double v2 = m_g * bp;
    const double lp = v2 + m_s2;
    m_s2 = v2 + lp;

    if (mode == 0)
        return static_cast<float>(lp);
    if (mode == 1)
        return static_cast<float>(hp);
    if (mode == 2)
        return static_cast<float>(bp);
    return input;
}

} // namespace noteahead
