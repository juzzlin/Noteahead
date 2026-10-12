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
#include "fm_drums_presets.hpp"

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
using PadVoicing = FmDrumsPadVoicing;

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

//! @p position as the position a project saves: on the grid of 10000 steps that a continuous
//! control is written on. The kit is written in hertz and seconds, whose positions fall between
//! those steps, and unsnapped a fresh pad would sound a hair different once saved and opened again.
float snapped(double position)
{
    return Parameter::xmlValueToInternal(Parameter::internalToXmlValue(static_cast<float>(position), 0, 10000), 0, 10000);
}

//! One engine control of a pad as a voicing sets it.
struct EngineValue
{
    QString key;
    //! The knob position for a continuous control, the plain index for a choice.
    double value;
    //! How many choices, or zero for a continuous control.
    size_t choices;
};

//! Every engine control of a pad and the value @p pad gives it. The one list both the parameters
//! and the presets are made from, so a control cannot be registered and then missed by a preset.
std::vector<EngineValue> engineValues(const PadVoicing & pad)
{
    std::vector<EngineValue> values {
        { NahdXml::xmlKeyAlgorithm(), static_cast<double>(pad.algorithm), FmSynthDevice::AlgorithmCount },
        { NahdXml::xmlKeyFrequency(), positionOfFrequency(pad.frequency), 0 },
        { NahdXml::xmlKeyFeedback(), pad.feedback, 0 },
        { NahdXml::xmlKeyPitchDepth(), positionOfSemitones(pad.pitchSemitones), 0 },
        { NahdXml::xmlKeyPitchDecay(), positionOfPitchDecay(pad.pitchDecay), 0 },
    };
    for (size_t op = 0; op < OperatorCount; op++) {
        const auto & voicing = pad.operators.at(op);
        values.push_back({ NahdXml::xmlKeyOperatorWaveform(op), static_cast<double>(voicing.waveform), FmOperator::WaveformCount });
        values.push_back({ NahdXml::xmlKeyOperatorRatio(op), positionOfRatio(voicing.ratio), 0 });
        values.push_back({ NahdXml::xmlKeyOperatorLevel(op), voicing.level, 0 });
        values.push_back({ NahdXml::xmlKeyOperatorAttack(op), positionOfAttack(voicing.attack), 0 });
        values.push_back({ NahdXml::xmlKeyOperatorDecay(op), positionOfDecay(voicing.decay), 0 });
        values.push_back({ NahdXml::xmlKeyOperatorCurve(op), voicing.curve, 0 });
    }
    values.push_back({ NahdXml::xmlKeyNoiseLevel(), pad.noiseLevel, 0 });
    values.push_back({ NahdXml::xmlKeyNoiseModulation(), pad.noiseModulation, 0 });
    values.push_back({ NahdXml::xmlKeyNoiseTarget(), static_cast<double>(pad.noiseTarget), OperatorCount });
    values.push_back({ NahdXml::xmlKeyNoiseAttack(), positionOfAttack(pad.noiseAttack), 0 });
    values.push_back({ NahdXml::xmlKeyNoiseDecay(), positionOfDecay(pad.noiseDecay), 0 });
    values.push_back({ NahdXml::xmlKeyNoiseCurve(), pad.noiseCurve, 0 });
    values.push_back({ NahdXml::xmlKeyNoiseFilterMode(), static_cast<double>(pad.noiseFilterMode), FmDrumEngine::NoiseFilterModeCount });
    values.push_back({ NahdXml::xmlKeyNoiseCutoff(), positionOfNoiseCutoff(pad.noiseCutoffHz), 0 });
    values.push_back({ NahdXml::xmlKeyNoiseResonance(), pad.noiseResonance, 0 });
    values.push_back({ NahdXml::xmlKeyVelocitySensitivity(), pad.velocityToLevel, 0 });
    values.push_back({ NahdXml::xmlKeyModVelocitySensitivity(), pad.velocityToModulation, 0 });
    return values;
}

//! The value a parameter stores for @p value: snapped for a knob, clamped to the choices otherwise.
float storedValue(const EngineValue & value)
{
    if (value.choices) {
        return static_cast<float>(std::clamp(std::round(value.value), 0.0, static_cast<double>(value.choices - 1)));
    }
    return snapped(std::clamp(value.value, 0.0, 1.0));
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
    for (auto && value : engineValues(FmDrumsPresets::kits().front().pads.at(static_cast<size_t>(index)))) {
        const auto key = prefix + value.key.toStdString();
        const float stored = storedValue(value);
        if (value.choices) {
            addParameter(Parameter { key, stored, 0, static_cast<int>(value.choices) - 1, static_cast<int>(stored), 1, Parameter::Type::Discrete });
        } else {
            addParameter(Parameter { key, stored, 0, 10000, Parameter::internalToXmlValue(stored, 0, 10000), 100 });
        }
    }
}

std::pair<float, float> FmDrumsDevice::defaultFilterCutoffs(int index) const
{
    const auto & pad = FmDrumsPresets::kits().front().pads.at(static_cast<size_t>(index));
    return { snapped(pad.lpfCutoff), snapped(pad.hpfCutoff) };
}

void FmDrumsDevice::applyVoicing(int index, const FmDrumsPadVoicing & voicing)
{
    const std::string prefix { voiceId(index) + "_" };
    if (auto p = parameter(prefix + NahdXml::xmlKeyCutoff().toStdString()); p) {
        p->get().setValue(snapped(voicing.lpfCutoff));
    }
    if (auto p = parameter(prefix + NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        p->get().setValue(snapped(voicing.hpfCutoff));
    }
    for (auto && value : engineValues(voicing)) {
        if (auto p = parameter(prefix + value.key.toStdString()); p) {
            p->get().setValue(storedValue(value));
        }
    }
}

void FmDrumsDevice::loadKit(size_t index)
{
    const auto & kits = FmDrumsPresets::kits();
    if (index >= kits.size()) {
        return;
    }
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        // A kit is the whole device, so everything it does not set goes back to its default first:
        // the mix, the envelopes and the slopes along with the voices
        reset();
        for (int pad = 0; pad < NumVoices; pad++) {
            applyVoicing(pad, kits.at(index).pads.at(static_cast<size_t>(pad)));
        }
        syncParameters();
    }
    emit dataChanged();
}

void FmDrumsDevice::loadPadPreset(int pad, size_t index)
{
    const auto & pads = FmDrumsPresets::pads();
    if (pad < 0 || pad >= NumVoices || index >= pads.size()) {
        return;
    }
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        // The sound only: the pad keeps its level, pan, envelope and effects, which are where it
        // sits in the kit rather than what it is
        applyVoicing(pad, pads.at(index).voicing);
        syncParameters();
    }
    emit dataChanged();
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
