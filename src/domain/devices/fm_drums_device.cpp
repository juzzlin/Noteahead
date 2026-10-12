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

#include "fm_drums_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../tracker/parameter_display.hpp"
#include "fm_synth_device.hpp"

#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

using namespace DrumSynthV2;
namespace NahdXml = Constants::NahdXml;

namespace {

constexpr size_t OperatorCount { FmDrumEngine::OperatorCount };

//! Bottom and top of the noise filter's travel, as CascadedSvf maps it.
constexpr double MinNoiseCutoffHz { 20.0 };
constexpr double MaxNoiseCutoffHz { 20000.0 };

using Waveform = FmOperator::Waveform;
using NoiseFilterMode = FmDrumEngine::NoiseFilterMode;

struct OperatorVoicing
{
    Waveform waveform { Waveform::Sine };
    double ratio { 1.0 };
    //! Knob position: the engine gets its square, as the FM Synth's operators do.
    double level { 0.0 };
    double attack { 0.0 };
    double decay { 0.3 };
    double curve { 0.5 };
};

//! One pad as the kit ships it, in the units the controls read out in.
struct PadVoicing
{
    size_t algorithm { 0 };
    double frequency { 100.0 };
    double feedback { 0.0 };
    double pitchSemitones { 0.0 };
    double pitchDecay { 0.03 };
    std::array<OperatorVoicing, OperatorCount> operators {};
    double noiseLevel { 0.0 };
    double noiseModulation { 0.0 };
    size_t noiseTarget { 0 };
    double noiseAttack { 0.0 };
    double noiseDecay { 0.1 };
    double noiseCurve { 0.5 };
    NoiseFilterMode noiseFilterMode { NoiseFilterMode::HighPass };
    double noiseCutoffHz { 5000.0 };
    double noiseResonance { 0.0 };
    double velocityToLevel { 1.0 };
    double velocityToModulation { 0.5 };
};

// Algorithm ordinals, as FmSynthDevice::algorithms() lists them
constexpr size_t Serial { 0 };
constexpr size_t TwinStacks { 4 };

//! The kit a new FM Drums starts with, in pad order: a starting point for every drum, built the way
//! FM drum machines have always built them. A sine falling in pitch for the drums, inharmonic
//! operator stacks for the metal, and noise for whatever FM cannot rattle on its own.
const std::array<PadVoicing, NumVoices> & defaultKit()
{
    static const std::array<PadVoicing, NumVoices> kit { {
      // Kick: a sine diving three octaves, and a modulator gone in forty milliseconds for the beater
      { .algorithm = Serial,
        .frequency = 48.0,
        .pitchSemitones = 30.0,
        .pitchDecay = 0.025,
        .operators = { { { .level = 1.0, .decay = 1.2, .curve = 0.6 },
                         { .ratio = 1.0, .level = 0.45, .decay = 0.04, .curve = 0.7 },
                         {},
                         {} } } },
      // Snare: a short pitched body, with band passed noise both heard and rattling the body
      { .algorithm = Serial,
        .frequency = 185.0,
        .pitchSemitones = 7.0,
        .pitchDecay = 0.02,
        .operators = { { { .level = 0.8, .decay = 0.3, .curve = 0.6 },
                         { .ratio = 1.6, .level = 0.4, .decay = 0.05, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.4,
        .noiseModulation = 0.25,
        .noiseDecay = 0.4,
        .noiseCurve = 0.6,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 4500.0,
        .noiseResonance = 0.2 },
      // Closed hi-hat: two inharmonic stacks and high passed noise, both gone at once
      { .algorithm = TwinStacks,
        .frequency = 610.0,
        .feedback = 0.3,
        .operators = { { { .level = 0.8, .decay = 0.1, .curve = 0.7 },
                         { .ratio = 1.47, .level = 0.85, .decay = 0.08 },
                         { .ratio = 2.31, .level = 0.7, .decay = 0.1, .curve = 0.7 },
                         { .ratio = 3.71, .level = 0.85, .decay = 0.08 } } },
        .noiseLevel = 0.5,
        .noiseDecay = 0.1,
        .noiseCurve = 0.7,
        .noiseCutoffHz = 7000.0 },
      // Clap: noise round two kilohertz, rattling a quiet sine for a body
      { .algorithm = Serial,
        .frequency = 1100.0,
        .operators = { { { .level = 0.35, .decay = 0.1, .curve = 0.6 }, {}, {}, {} } },
        .noiseLevel = 1.0,
        .noiseModulation = 0.8,
        .noiseDecay = 0.5,
        .noiseCurve = 0.7,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 1800.0,
        .noiseResonance = 0.3 },
      // Open hi-hat: the closed one, let ring
      { .algorithm = TwinStacks,
        .frequency = 610.0,
        .feedback = 0.3,
        .operators = { { { .level = 0.8, .decay = 0.8, .curve = 0.6 },
                         { .ratio = 1.47, .level = 0.85, .decay = 0.5 },
                         { .ratio = 2.31, .level = 0.7, .decay = 0.8, .curve = 0.6 },
                         { .ratio = 3.71, .level = 0.85, .decay = 0.5 } } },
        .noiseLevel = 0.5,
        .noiseDecay = 0.8,
        .noiseCurve = 0.6,
        .noiseCutoffHz = 7000.0 },
      // Low tom
      { .algorithm = Serial,
        .frequency = 82.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.05,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      // Mid tom
      { .algorithm = Serial,
        .frequency = 120.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.045,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      // High tom
      { .algorithm = Serial,
        .frequency = 170.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.04,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      // Crash: deep inharmonic stacks with feedback, and a long wash of noise
      { .algorithm = TwinStacks,
        .frequency = 310.0,
        .feedback = 0.5,
        .operators = { { { .level = 0.35, .decay = 3.0, .curve = 0.6 },
                         { .ratio = 2.76, .level = 0.9, .decay = 2.0 },
                         { .ratio = 1.53, .level = 0.3, .decay = 3.0, .curve = 0.6 },
                         { .ratio = 3.89, .level = 0.9, .decay = 2.0 } } },
        .noiseLevel = 0.2,
        .noiseModulation = 0.3,
        .noiseDecay = 2.5,
        .noiseCutoffHz = 5000.0 },
      // Ride: lighter modulation than the crash, so the bell stays in it
      { .algorithm = TwinStacks,
        .frequency = 420.0,
        .feedback = 0.2,
        .operators = { { { .level = 0.65, .decay = 4.5, .curve = 0.6 },
                         { .ratio = 3.17, .level = 0.55, .decay = 2.5 },
                         { .ratio = 1.39, .level = 0.5, .decay = 4.5, .curve = 0.6 },
                         { .ratio = 4.21, .level = 0.55, .decay = 2.5 } } },
        .noiseLevel = 0.2,
        .noiseDecay = 3.0,
        .noiseCutoffHz = 8000.0 },
      // Reverse crash: the crash with its attack and decay swapped round
      { .algorithm = TwinStacks,
        .frequency = 310.0,
        .feedback = 0.5,
        .operators = { { { .level = 0.35, .attack = 0.7, .decay = 0.04, .curve = 0.3 },
                         { .ratio = 2.76, .level = 0.9, .attack = 0.7, .decay = 0.06 },
                         { .ratio = 1.53, .level = 0.3, .attack = 0.7, .decay = 0.04, .curve = 0.3 },
                         { .ratio = 3.89, .level = 0.9, .attack = 0.7, .decay = 0.06 } } },
        .noiseLevel = 0.2,
        .noiseModulation = 0.3,
        .noiseAttack = 0.7,
        .noiseDecay = 0.04,
        .noiseCurve = 0.3,
        .noiseCutoffHz = 5000.0 },
      // Rim: a high, hard and very short click
      { .algorithm = Serial,
        .frequency = 520.0,
        .operators = { { { .level = 1.0, .decay = 0.035, .curve = 0.7 },
                         { .ratio = 2.3, .level = 0.5, .decay = 0.012, .curve = 0.7 },
                         {},
                         {} } },
        .noiseLevel = 0.3,
        .noiseDecay = 0.012,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 3500.0 },
    } };
    return kit;
}

// The knob travel of each kind of control, both ways
double frequencyFromPosition(double position)
{
    return ParameterMapper::mapExponential(position, FmDrumsDevice::MinFrequency, FmDrumsDevice::MaxFrequency);
}

double ratioFromPosition(double position)
{
    return ParameterMapper::mapExponential(position, FmDrumsDevice::MinRatio, FmDrumsDevice::MaxRatio);
}

double semitonesFromPosition(double position)
{
    return ParameterMapper::mapCubicCentered(position * 2.0 - 1.0, -FmDrumsDevice::MaxPitchSemitones, FmDrumsDevice::MaxPitchSemitones);
}

double pitchDecayFromPosition(double position)
{
    return ParameterMapper::mapExponential(position, FmDrumsDevice::MinPitchDecaySeconds, FmDrumsDevice::MaxPitchDecaySeconds);
}

double attackFromPosition(double position)
{
    return ParameterMapper::mapCubic(position, 0.0, FmDrumsDevice::MaxAttackSeconds);
}

double decayFromPosition(double position)
{
    return ParameterMapper::mapExponential(position, FmDrumsDevice::MinDecaySeconds, FmDrumsDevice::MaxDecaySeconds);
}

float positionOfFrequency(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, FmDrumsDevice::MinFrequency, FmDrumsDevice::MaxFrequency));
}

float positionOfRatio(double ratio)
{
    return static_cast<float>(ParameterMapper::unmapExponential(ratio, FmDrumsDevice::MinRatio, FmDrumsDevice::MaxRatio));
}

float positionOfSemitones(double semitones)
{
    return static_cast<float>((ParameterMapper::unmapCubicCentered(semitones, -FmDrumsDevice::MaxPitchSemitones, FmDrumsDevice::MaxPitchSemitones) + 1.0) / 2.0);
}

float positionOfPitchDecay(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, FmDrumsDevice::MinPitchDecaySeconds, FmDrumsDevice::MaxPitchDecaySeconds));
}

float positionOfAttack(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapCubic(seconds, 0.0, FmDrumsDevice::MaxAttackSeconds));
}

float positionOfDecay(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, FmDrumsDevice::MinDecaySeconds, FmDrumsDevice::MaxDecaySeconds));
}

float positionOfNoiseCutoff(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, MinNoiseCutoffHz, MaxNoiseCutoffHz));
}

//! A control resting at @p position, snapped onto the grid a project saves positions on. The kit is
//! written in hertz and seconds, whose positions fall between the steps of that grid: unsnapped, a
//! fresh device would sound a hair different once saved and opened again.
Parameter continuous(const std::string & key, double position)
{
    const int xmlValue = Parameter::internalToXmlValue(static_cast<float>(position), 0, 10000);
    return Parameter { key, Parameter::xmlValueToInternal(xmlValue, 0, 10000), 0, 10000, xmlValue, 100 };
}

Parameter discrete(const std::string & key, size_t value, size_t count)
{
    return Parameter { key, static_cast<float>(value), 0, static_cast<int>(count) - 1, static_cast<int>(value), 1, Parameter::Type::Discrete };
}

} // namespace

FmDrumsDevice::FmDrumsDevice(std::string name)
  : DrumPadDevice { std::move(name) }
{
    for (int i { 0 }; i < NumVoices; i++) {
        setVoiceEngine(i, std::make_unique<FmDrumEngine>());
    }

    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
        // After the common descriptions, which read a few of these keys as the analogue drums do
        describeEngineParameters(voiceId(i) + "_");
    }

    FmDrumsDevice::syncParameters();
}

std::string FmDrumsDevice::typeName() const
{
    return Constants::fmDrumsDeviceName().toStdString();
}

std::string FmDrumsDevice::typeIdString()
{
    return "824a981c-f615-4485-8e54-bc4ad8f5972a";
}

std::string FmDrumsDevice::typeId() const
{
    return typeIdString();
}

std::unique_ptr<DrumPadDevice> FmDrumsDevice::newInstance(std::string name) const
{
    return std::make_unique<FmDrumsDevice>(std::move(name));
}

FmDrumEngine & FmDrumsDevice::engine(int index)
{
    return static_cast<FmDrumEngine &>(voiceEngine(index));
}

void FmDrumsDevice::addEngineParameters(int index, const std::string & prefix)
{
    const auto & pad = defaultKit().at(static_cast<size_t>(index));
    const auto key = [&prefix](const QString & name) {
        return prefix + name.toStdString();
    };

    addParameter(discrete(key(NahdXml::xmlKeyAlgorithm()), pad.algorithm, FmSynthDevice::AlgorithmCount));
    addParameter(continuous(key(NahdXml::xmlKeyFrequency()), positionOfFrequency(pad.frequency)));
    addParameter(continuous(key(NahdXml::xmlKeyFeedback()), pad.feedback));
    addParameter(continuous(key(NahdXml::xmlKeyPitchDepth()), positionOfSemitones(pad.pitchSemitones)));
    addParameter(continuous(key(NahdXml::xmlKeyPitchDecay()), positionOfPitchDecay(pad.pitchDecay)));

    for (size_t op = 0; op < OperatorCount; op++) {
        const auto & voicing = pad.operators.at(op);
        addParameter(discrete(key(NahdXml::xmlKeyOperatorWaveform(op)), static_cast<size_t>(voicing.waveform), FmOperator::WaveformCount));
        addParameter(continuous(key(NahdXml::xmlKeyOperatorRatio(op)), positionOfRatio(voicing.ratio)));
        addParameter(continuous(key(NahdXml::xmlKeyOperatorLevel(op)), voicing.level));
        addParameter(continuous(key(NahdXml::xmlKeyOperatorAttack(op)), positionOfAttack(voicing.attack)));
        addParameter(continuous(key(NahdXml::xmlKeyOperatorDecay(op)), positionOfDecay(voicing.decay)));
        addParameter(continuous(key(NahdXml::xmlKeyOperatorCurve(op)), voicing.curve));
    }

    addParameter(continuous(key(NahdXml::xmlKeyNoiseLevel()), pad.noiseLevel));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseModulation()), pad.noiseModulation));
    addParameter(discrete(key(NahdXml::xmlKeyNoiseTarget()), pad.noiseTarget, OperatorCount));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseAttack()), positionOfAttack(pad.noiseAttack)));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseDecay()), positionOfDecay(pad.noiseDecay)));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseCurve()), pad.noiseCurve));
    addParameter(discrete(key(NahdXml::xmlKeyNoiseFilterMode()), static_cast<size_t>(pad.noiseFilterMode), FmDrumEngine::NoiseFilterModeCount));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseCutoff()), positionOfNoiseCutoff(pad.noiseCutoffHz)));
    addParameter(continuous(key(NahdXml::xmlKeyNoiseResonance()), pad.noiseResonance));

    addParameter(continuous(key(NahdXml::xmlKeyVelocitySensitivity()), pad.velocityToLevel));
    addParameter(continuous(key(NahdXml::xmlKeyModVelocitySensitivity()), pad.velocityToModulation));
}

void FmDrumsDevice::syncEngineParameters(int index, const std::string & prefix)
{
    auto & drum = engine(index);
    const auto value = [this, &prefix](const QString & name) -> std::optional<double> {
        if (const auto p = parameter(prefix + name.toStdString()); p) {
            return static_cast<double>(p->get().value());
        }
        return std::nullopt;
    };

    if (const auto v = value(NahdXml::xmlKeyAlgorithm()); v) {
        const auto & algorithms = FmSynthDevice::algorithms();
        const auto & algorithm = algorithms.at(std::clamp(static_cast<size_t>(std::lround(*v)), size_t { 0 }, algorithms.size() - 1));
        drum.setAlgorithm(algorithm.modulators, algorithm.carriers);
    }
    if (const auto v = value(NahdXml::xmlKeyFrequency()); v) {
        drum.setBaseFrequency(frequencyFromPosition(*v));
    }
    if (const auto v = value(NahdXml::xmlKeyFeedback()); v) {
        drum.setFeedback(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyPitchDepth()); v) {
        drum.setPitchEnvelopeAmount(semitonesFromPosition(*v));
    }
    if (const auto v = value(NahdXml::xmlKeyPitchDecay()); v) {
        drum.setPitchEnvelopeDecay(pitchDecayFromPosition(*v));
    }

    for (size_t op = 0; op < OperatorCount; op++) {
        if (const auto v = value(NahdXml::xmlKeyOperatorWaveform(op)); v) {
            drum.setOperatorWaveform(op, static_cast<Waveform>(std::clamp(static_cast<size_t>(std::lround(*v)), size_t { 0 }, FmOperator::WaveformCount - 1)));
        }
        if (const auto v = value(NahdXml::xmlKeyOperatorRatio(op)); v) {
            drum.setOperatorRatio(op, ratioFromPosition(*v));
        }
        if (const auto v = value(NahdXml::xmlKeyOperatorLevel(op)); v) {
            drum.setOperatorLevel(op, *v * *v);
        }
        if (const auto v = value(NahdXml::xmlKeyOperatorAttack(op)); v) {
            drum.setOperatorAttack(op, attackFromPosition(*v));
        }
        if (const auto v = value(NahdXml::xmlKeyOperatorDecay(op)); v) {
            drum.setOperatorDecay(op, decayFromPosition(*v));
        }
        if (const auto v = value(NahdXml::xmlKeyOperatorCurve(op)); v) {
            drum.setOperatorCurve(op, *v);
        }
    }

    if (const auto v = value(NahdXml::xmlKeyNoiseLevel()); v) {
        drum.setNoiseLevel(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseModulation()); v) {
        drum.setNoiseModulation(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseTarget()); v) {
        drum.setNoiseTarget(static_cast<size_t>(std::max(0L, std::lround(*v))));
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseAttack()); v) {
        drum.setNoiseAttack(attackFromPosition(*v));
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseDecay()); v) {
        drum.setNoiseDecay(decayFromPosition(*v));
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseCurve()); v) {
        drum.setNoiseCurve(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseFilterMode()); v) {
        drum.setNoiseFilterMode(static_cast<NoiseFilterMode>(std::clamp(static_cast<size_t>(std::lround(*v)), size_t { 0 }, FmDrumEngine::NoiseFilterModeCount - 1)));
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseCutoff()); v) {
        drum.setNoiseCutoff(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyNoiseResonance()); v) {
        drum.setNoiseResonance(*v);
    }

    if (const auto v = value(NahdXml::xmlKeyVelocitySensitivity()); v) {
        drum.setVelocityToLevel(*v);
    }
    if (const auto v = value(NahdXml::xmlKeyModVelocitySensitivity()); v) {
        drum.setVelocityToModulation(*v);
    }
}

void FmDrumsDevice::describeEngineParameters(const std::string & prefix)
{
    using Display = ParameterDisplay;
    const auto describe = [&](const QString & key, Display display) {
        setParameterDisplay(prefix + key.toStdString(), std::move(display));
    };
    const auto percent = [&](const QString & key, const char * label) {
        describe(key, { .unit = "%", .max = 100.0, .label = label });
    };
    const auto attack = [&](const QString & key, const char * label) {
        describe(key, { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = MaxAttackSeconds, .label = label });
    };
    const auto decay = [&](const QString & key, const char * label) {
        describe(key, { .mapping = Display::Mapping::Exponential, .unit = "s", .min = MinDecaySeconds, .max = MaxDecaySeconds, .label = label });
    };

    describe(NahdXml::xmlKeyFrequency(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = MinFrequency, .max = MaxFrequency, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch") });
    percent(NahdXml::xmlKeyFeedback(), QT_TRANSLATE_NOOP("ParameterDisplay", "Feedback"));
    describe(NahdXml::xmlKeyPitchDepth(), { .mapping = Display::Mapping::CubicCentered, .unit = "st", .min = -MaxPitchSemitones, .max = MaxPitchSemitones, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth") });
    describe(NahdXml::xmlKeyPitchDecay(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = MinPitchDecaySeconds, .max = MaxPitchDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Decay") });

    // Spelled out per operator: the labels are translated where they are shown, and only literals
    // can be marked for that
    static constexpr std::array<std::array<const char *, 5>, OperatorCount> operatorLabels { {
      { QT_TRANSLATE_NOOP("ParameterDisplay", "Op 1 Ratio"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 1 Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 1 Attack"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 1 Decay"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 1 Curve") },
      { QT_TRANSLATE_NOOP("ParameterDisplay", "Op 2 Ratio"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 2 Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 2 Attack"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 2 Decay"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 2 Curve") },
      { QT_TRANSLATE_NOOP("ParameterDisplay", "Op 3 Ratio"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 3 Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 3 Attack"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 3 Decay"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 3 Curve") },
      { QT_TRANSLATE_NOOP("ParameterDisplay", "Op 4 Ratio"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 4 Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 4 Attack"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 4 Decay"), QT_TRANSLATE_NOOP("ParameterDisplay", "Op 4 Curve") },
    } };
    for (size_t op = 0; op < OperatorCount; op++) {
        const auto & labels = operatorLabels.at(op);
        describe(NahdXml::xmlKeyOperatorRatio(op), { .mapping = Display::Mapping::Exponential, .unit = "", .min = MinRatio, .max = MaxRatio, .label = labels.at(0) });
        percent(NahdXml::xmlKeyOperatorLevel(op), labels.at(1));
        attack(NahdXml::xmlKeyOperatorAttack(op), labels.at(2));
        decay(NahdXml::xmlKeyOperatorDecay(op), labels.at(3));
        percent(NahdXml::xmlKeyOperatorCurve(op), labels.at(4));
    }

    percent(NahdXml::xmlKeyNoiseLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Level"));
    percent(NahdXml::xmlKeyNoiseModulation(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise FM"));
    attack(NahdXml::xmlKeyNoiseAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Attack"));
    decay(NahdXml::xmlKeyNoiseDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Decay"));
    percent(NahdXml::xmlKeyNoiseCurve(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Curve"));
    describe(NahdXml::xmlKeyNoiseCutoff(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = MinNoiseCutoffHz, .max = MaxNoiseCutoffHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Cutoff") });
    percent(NahdXml::xmlKeyNoiseResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Resonance"));

    percent(NahdXml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity"));
    percent(NahdXml::xmlKeyModVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity to FM"));
}

} // namespace noteahead
