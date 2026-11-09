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

#include "synth_d_device.hpp"

#include "device_parameter_display.hpp"
#include "synth_d_presets.hpp"
#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

//! Brings Init to about -26 dBFS, where the JU Synth 08 plays
constexpr double OutputGain = 0.25;

//! The knobs, 0..1, with their defaults: one sawtooth through a half-open filter, a patch to start from
struct Knob
{
    QString key;
    float defaultValue;
};

const std::vector<Knob> & knobs()
{
    static const std::vector<Knob> knobs {
        { Xml::xmlKeyTune(), 0.5f },
        { Xml::xmlKeyGlide(), 0.0f },
        { Xml::xmlKeyModMix(), 0.0f },
        { Xml::xmlKeyModDepth(), 0.0f },
        { Xml::xmlKeyLfoRate(), 0.5f },
        { Xml::xmlKeyVco2Pitch(), 0.5f },
        { Xml::xmlKeyVco3Pitch(), 0.5f },
        { Xml::xmlKeyVco1Level(), 1.0f },
        { Xml::xmlKeyVco2Level(), 1.0f },
        { Xml::xmlKeyVco3Level(), 1.0f },
        { Xml::xmlKeyFeedback(), 0.0f },
        { Xml::xmlKeyNoiseLevel(), 0.0f },
        { Xml::xmlKeyCutoff(), 0.6f },
        { Xml::xmlKeyResonance(), 0.0f },
        { Xml::xmlKeyEnvMod(), 0.3f },
        { Xml::xmlKeyEnv1Attack(), 0.0f },
        { Xml::xmlKeyEnv1Decay(), 0.4f },
        { Xml::xmlKeyEnv1Sustain(), 0.5f },
        { Xml::xmlKeyEnv2Attack(), 0.0f },
        { Xml::xmlKeyEnv2Decay(), 0.4f },
        { Xml::xmlKeyEnv2Sustain(), 1.0f },
        { Xml::xmlKeyOscillatorDrift(), 0.15f },
        { Xml::xmlKeyOscillatorInstability(), 0.15f },
        { Xml::xmlKeyVoiceTolerance(), 0.25f },
        { Xml::xmlKeyPanSpread(), 0.0f },
        { Xml::xmlKeyVelocitySensitivity(), 0.0f },
        { Xml::xmlKeyLpfCutoff(), 1.0f },
        { Xml::xmlKeyHpfCutoff(), 0.0f },
    };
    return knobs;
}

//! The switches and selectors, with their defaults and how many positions they have
struct Choice
{
    QString key;
    int defaultValue;
    int count;
};

const std::vector<Choice> & choices()
{
    using V = SynthDVoice;
    static const std::vector<Choice> choices {
        { Xml::xmlKeyModSource1(), static_cast<int>(V::ModSource1::Osc3), 2 },
        { Xml::xmlKeyModSource2(), static_cast<int>(V::ModSource2::Noise), 2 },
        { Xml::xmlKeyOscillatorModulation(), 0, 2 },
        { Xml::xmlKeyFilterModulation(), 0, 2 },
        { Xml::xmlKeyLfoWaveform(), static_cast<int>(V::LfoWaveform::Triangle), 2 },
        { Xml::xmlKeyVco1Range(), static_cast<int>(V::Range::Feet8), 6 },
        { Xml::xmlKeyVco2Range(), static_cast<int>(V::Range::Feet8), 6 },
        { Xml::xmlKeyVco3Range(), static_cast<int>(V::Range::Feet8), 6 },
        { Xml::xmlKeyVco1Waveform(), static_cast<int>(V::Waveform::Saw), 6 },
        { Xml::xmlKeyVco2Waveform(), static_cast<int>(V::Waveform::Saw), 6 },
        { Xml::xmlKeyVco3Waveform(), static_cast<int>(V::Waveform::Saw), 6 },
        { Xml::xmlKeyVco3KeyboardControl(), 1, 2 },
        { Xml::xmlKeyVco1On(), 1, 2 },
        { Xml::xmlKeyVco2On(), 0, 2 },
        { Xml::xmlKeyVco3On(), 0, 2 },
        { Xml::xmlKeyFeedbackOn(), 0, 2 },
        { Xml::xmlKeyNoiseOn(), 0, 2 },
        { Xml::xmlKeyColor(), static_cast<int>(V::NoiseColour::White), 2 },
        { Xml::xmlKeyFilterType(), static_cast<int>(TransistorLadderFilter::Mode::LowPass), 2 },
        { Xml::xmlKeyKeyboardControl1(), 1, 2 },
        { Xml::xmlKeyKeyboardControl2(), 0, 2 },
        { Xml::xmlKeyEnv1DecaySwitch(), 0, 2 },
        { Xml::xmlKeyEnv2DecaySwitch(), 0, 2 },
        { Xml::xmlKeyNotePriority(), static_cast<int>(SynthDDevice::NotePriority::Low), 3 },
        { Xml::xmlKeyRetrigger(), 0, 2 },
    };
    return choices;
}

struct CcParameter
{
    uint8_t controller {};
    std::string label;
    std::string key;
};

const std::vector<CcParameter> & ccParameters()
{
    using namespace MidiCcMapping;
    static const std::vector<CcParameter> parameters {
        { .controller = static_cast<uint8_t>(Controller::ModulationWheelMSB), .label = "Mod Depth", .key = Xml::xmlKeyModDepth().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::PortamentoTimeMSB), .label = "Glide", .key = Xml::xmlKeyGlide().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Emphasis", .key = Xml::xmlKeyResonance().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Loudness Decay", .key = Xml::xmlKeyEnv2Decay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Loudness Attack", .key = Xml::xmlKeyEnv2Attack().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Amount of Contour", .key = Xml::xmlKeyEnvMod().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Filter Decay", .key = Xml::xmlKeyEnv1Decay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController8), .label = "Mod Mix", .key = Xml::xmlKeyModMix().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController9), .label = "Cutoff", .key = Xml::xmlKeyCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController10), .label = "LFO Rate", .key = Xml::xmlKeyLfoRate().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
    };
    return parameters;
}

} // namespace

SynthDDevice::SynthDDevice(std::string name)
  : m_name { std::move(name) }
{
    for (auto && knob : knobs()) {
        const auto defaultStep = static_cast<int>(std::lround(knob.defaultValue * 10000.0f));
        addParameter(Parameter(knob.key.toStdString(), knob.defaultValue, 0, 10000, defaultStep, 100));
    }
    for (auto && choice : choices()) {
        const auto type = choice.count == 2 ? Parameter::Type::Boolean : Parameter::Type::Discrete;
        addParameter(Parameter(choice.key.toStdString(), static_cast<float>(choice.defaultValue), 0, choice.count - 1, choice.defaultValue, 1, type));
    }
    addParameter(Parameter(Xml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete));

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);
    m_voice.reset();

    SynthDDevice::syncParameters();

    using Display = ParameterDisplay;
    DeviceParameterDisplay::filters(*this);
    setParameterDisplay(Xml::xmlKeyTune().toStdString(), { .mapping = Display::Mapping::Bipolar, .unit = "semitones", .min = -2.0, .max = 2.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Tune") });
    setParameterDisplay(Xml::xmlKeyVco2Pitch().toStdString(), { .mapping = Display::Mapping::Bipolar, .unit = "semitones", .min = -7.0, .max = 7.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "OSC 2 Frequency") });
    setParameterDisplay(Xml::xmlKeyVco3Pitch().toStdString(), { .mapping = Display::Mapping::Bipolar, .unit = "semitones", .min = -7.0, .max = 7.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "OSC 3 Frequency") });
    setParameterDisplay(Xml::xmlKeyLfoRate().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = 0.05, .max = 200.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO Rate") });
    setParameterDisplay(Xml::xmlKeyCutoff().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = SynthDVoice::LowestCutoffHz, .max = SynthDVoice::HighestCutoffHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Cutoff") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv1Attack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Filter Attack"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv1Decay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Filter Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv2Attack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Loudness Attack"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv2Decay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Loudness Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyGlide(), QT_TRANSLATE_NOOP("ParameterDisplay", "Glide"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyModMix(), QT_TRANSLATE_NOOP("ParameterDisplay", "Mod Mix"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyModDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Mod Depth"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVco1Level(), QT_TRANSLATE_NOOP("ParameterDisplay", "OSC 1 Volume"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVco2Level(), QT_TRANSLATE_NOOP("ParameterDisplay", "OSC 2 Volume"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVco3Level(), QT_TRANSLATE_NOOP("ParameterDisplay", "OSC 3 Volume"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyFeedback(), QT_TRANSLATE_NOOP("ParameterDisplay", "Ext In Volume"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyNoiseLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Volume"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Emphasis"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnvMod(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amount of Contour"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv1Sustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Filter Sustain"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyEnv2Sustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Loudness Sustain"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyOscillatorDrift(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drift"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyOscillatorInstability(), QT_TRANSLATE_NOOP("ParameterDisplay", "Instability"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVoiceTolerance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tolerance"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPanSpread(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pan Spread"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

SynthDDevice::~SynthDDevice() = default;

std::string SynthDDevice::name() const
{
    return m_name;
}

std::string SynthDDevice::category() const
{
    return Xml::xmlValueSynths().toStdString();
}

std::string SynthDDevice::typeName() const
{
    return Constants::synthDDeviceName().toStdString();
}

std::string SynthDDevice::typeIdString()
{
    return "7d3f9a12-6c4e-4b8d-a5f1-2e90c8b6d4a7";
}

std::string SynthDDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> SynthDDevice::deviceMidiCcControllers() const
{
    std::vector<MidiCcController> controllers;
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

std::optional<uint8_t> SynthDDevice::priorityNote() const
{
    if (m_heldKeys.empty()) {
        return {};
    }
    switch (m_notePriority) {
    case NotePriority::Low:
        return *std::ranges::min_element(m_heldKeys);
    case NotePriority::High:
        return *std::ranges::max_element(m_heldKeys);
    case NotePriority::Last:
        break;
    }
    return m_heldKeys.back();
}

void SynthDDevice::followKeys()
{
    const auto note = priorityNote();
    if (!note) {
        m_soundingNote.reset();
        m_voice.release();
        return;
    }
    const bool firstKey = !m_soundingNote;
    // A key that does not take priority changes nothing: the voice stays where it is
    if (!firstKey && *note == *m_soundingNote) {
        return;
    }
    // Legato moves the pitch under the contours; multi-trigger restarts them on every new note,
    // including the one a released key hands back to. A first key always starts them.
    m_voice.play(*note, m_velocity, firstKey || m_multiTrigger);
    m_soundingNote = note;
}

void SynthDDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    std::erase(m_heldKeys, note);
    m_heldKeys.push_back(note);
    const double v = static_cast<double>(velocity) / 127.0;
    m_velocity = 1.0 - static_cast<double>(m_velocitySensitivity) * (1.0 - v);
    m_voice.setSampleRate(sampleRate());
    followKeys();
}

void SynthDDevice::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    std::erase(m_heldKeys, note);
    followKeys();
}

void SynthDDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else {
                const auto & parameters = ccParameters();
                if (const auto it = std::ranges::find(parameters, controller, &CcParameter::controller); it != parameters.end()) {
                    if (const auto p = parameter(it->key); p) {
                        p->get().setAutomationValue(val);
                        syncParameters();
                        changed = true;
                    }
                }
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void SynthDDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_voice.setPitchBend(static_cast<double>(m_pitchBendRange) * (static_cast<double>(value) - 8192.0) / 8192.0);
}

void SynthDDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_heldKeys.clear();
    m_soundingNote.reset();
    m_voice.release();
}

void SynthDDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_voice.setSampleRate(static_cast<double>(context.sampleRate));
    m_lpfL.setSampleRate(context.sampleRate);
    m_lpfR.setSampleRate(context.sampleRate);
    m_hpfL.setSampleRate(context.sampleRate);
    m_hpfR.setSampleRate(context.sampleRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));
    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    m_panner.setPan(static_cast<double>(panInternal()));

    const double gain = OutputGain * linearGainInternal();
    const bool active = m_voice.isActive();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        // The LFO and the VCOs run whether a key is down or not, as on the instrument
        m_voice.process(left, right);
        if (!active) {
            continue;
        }
        left *= gain;
        right *= gain;
        m_panner.process(left, right);
        left = m_hpfL.process(m_lpfL.process(left));
        right = m_hpfR.process(m_lpfR.process(right));
        context.buffer[i * 2] += m_dcBlockerL.process(left);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(right);
    }

    publishLfoLed(LfoLed, m_voice.lfoValue());
    publishLfoLed(OverloadLed, 2.0 * m_voice.overload() - 1.0);
}

void SynthDDevice::advanceIdle(const AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_voice.setSampleRate(static_cast<double>(context.sampleRate));
    m_voice.advanceLfo(context.frameCount);
    publishLfoLed(LfoLed, m_voice.lfoValue());
    publishLfoLed(OverloadLed, -1.0);
}

bool SynthDDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voice.isActive();
}

void SynthDDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void SynthDDevice::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_heldKeys.clear();
    m_soundingNote.reset();
    m_voice.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void SynthDDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Xml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Xml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Xml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void SynthDDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Xml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Xml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Xml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Xml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

void SynthDDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void SynthDDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = SynthDPresets::presets();
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            return;
        }

        if (authored) {
            reset();
        } else {
            for (auto && [name, p] : parameters()) {
                p.setAutomationValue(p.defaultValue());
            }
        }

        for (auto && [name, value] : presets[static_cast<size_t>(index)].parameters) {
            if (const auto p = parameter(name); p) {
                if (authored) {
                    p->get().setValue(value);
                } else {
                    p->get().setAutomationValue(value);
                }
            }
        }

        syncParameters();
    }

    if (authored) {
        emit dataChanged();
    } else {
        emit parametersChanged();
    }
}

float SynthDDevice::value(const QString & key) const
{
    if (const auto p = parameter(key.toStdString()); p) {
        return p->get().value();
    }
    return 0.0f;
}

float SynthDDevice::knob(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return shownParameterValue(key, value(key));
}

void SynthDDevice::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

int SynthDDevice::choice(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return static_cast<int>(std::lround(value(key)));
}

void SynthDDevice::setChoice(const QString & key, int value)
{
    setDiscreteParameterValue(key.toStdString(), value);
}

std::optional<uint8_t> SynthDDevice::soundingNote() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_soundingNote;
}

void SynthDDevice::syncParameters()
{
    Device::syncParameters();

    using V = SynthDVoice;
    const auto knobValue = [this](const QString & key) {
        return static_cast<double>(value(key));
    };
    const auto choiceValue = [this](const QString & key, int count) {
        return std::clamp(static_cast<int>(std::lround(value(key))), 0, count - 1);
    };
    const auto on = [&](const QString & key) {
        return choiceValue(key, 2) == 1;
    };
    // The bipolar knobs are stored 0..1 about their middle
    const auto bipolar = [&](const QString & key, double range) {
        return (knobValue(key) * 2.0 - 1.0) * range;
    };

    auto & s = m_settings;
    s.tune = bipolar(Xml::xmlKeyTune(), 2.0);
    s.glide = knobValue(Xml::xmlKeyGlide());
    s.modMix = knobValue(Xml::xmlKeyModMix());
    s.modDepth = knobValue(Xml::xmlKeyModDepth());
    s.modSource1 = static_cast<V::ModSource1>(choiceValue(Xml::xmlKeyModSource1(), 2));
    s.modSource2 = static_cast<V::ModSource2>(choiceValue(Xml::xmlKeyModSource2(), 2));
    s.oscillatorModulation = on(Xml::xmlKeyOscillatorModulation());
    s.filterModulation = on(Xml::xmlKeyFilterModulation());
    s.lfoWaveform = static_cast<V::LfoWaveform>(choiceValue(Xml::xmlKeyLfoWaveform(), 2));
    s.lfoRate = knobValue(Xml::xmlKeyLfoRate());

    const std::array<QString, 3> ranges { Xml::xmlKeyVco1Range(), Xml::xmlKeyVco2Range(), Xml::xmlKeyVco3Range() };
    const std::array<QString, 3> waveforms { Xml::xmlKeyVco1Waveform(), Xml::xmlKeyVco2Waveform(), Xml::xmlKeyVco3Waveform() };
    const std::array<QString, 3> volumes { Xml::xmlKeyVco1Level(), Xml::xmlKeyVco2Level(), Xml::xmlKeyVco3Level() };
    const std::array<QString, 3> switches { Xml::xmlKeyVco1On(), Xml::xmlKeyVco2On(), Xml::xmlKeyVco3On() };
    for (size_t i = 0; i < 3; i++) {
        auto & oscillator = s.oscillators[i];
        oscillator.range = static_cast<V::Range>(choiceValue(ranges[i], 6));
        oscillator.waveform = static_cast<V::Waveform>(choiceValue(waveforms[i], 6));
        oscillator.volume = knobValue(volumes[i]);
        oscillator.on = on(switches[i]);
    }
    s.oscillators[0].frequency = 0.0;
    s.oscillators[1].frequency = bipolar(Xml::xmlKeyVco2Pitch(), 7.0);
    s.oscillators[2].frequency = bipolar(Xml::xmlKeyVco3Pitch(), 7.0);
    s.osc3KeyboardControl = on(Xml::xmlKeyVco3KeyboardControl());

    s.feedback = knobValue(Xml::xmlKeyFeedback());
    s.feedbackOn = on(Xml::xmlKeyFeedbackOn());
    s.noise = knobValue(Xml::xmlKeyNoiseLevel());
    s.noiseOn = on(Xml::xmlKeyNoiseOn());
    s.noiseColour = static_cast<V::NoiseColour>(choiceValue(Xml::xmlKeyColor(), 2));

    s.filterMode = static_cast<TransistorLadderFilter::Mode>(choiceValue(Xml::xmlKeyFilterType(), 2));
    s.cutoff = knobValue(Xml::xmlKeyCutoff());
    s.emphasis = knobValue(Xml::xmlKeyResonance());
    s.contourAmount = knobValue(Xml::xmlKeyEnvMod());
    s.keyboardControl1 = on(Xml::xmlKeyKeyboardControl1());
    s.keyboardControl2 = on(Xml::xmlKeyKeyboardControl2());
    s.filterContour = {
        .attack = knobValue(Xml::xmlKeyEnv1Attack()),
        .decay = knobValue(Xml::xmlKeyEnv1Decay()),
        .sustain = knobValue(Xml::xmlKeyEnv1Sustain()),
        .decaySwitch = on(Xml::xmlKeyEnv1DecaySwitch()),
    };
    s.loudnessContour = {
        .attack = knobValue(Xml::xmlKeyEnv2Attack()),
        .decay = knobValue(Xml::xmlKeyEnv2Decay()),
        .sustain = knobValue(Xml::xmlKeyEnv2Sustain()),
        .decaySwitch = on(Xml::xmlKeyEnv2DecaySwitch()),
    };

    s.drift = knobValue(Xml::xmlKeyOscillatorDrift());
    s.instability = knobValue(Xml::xmlKeyOscillatorInstability());
    s.tolerance = knobValue(Xml::xmlKeyVoiceTolerance());
    s.panSpread = knobValue(Xml::xmlKeyPanSpread());
    m_voice.setSettings(s);

    m_notePriority = static_cast<NotePriority>(choiceValue(Xml::xmlKeyNotePriority(), 3));
    m_multiTrigger = on(Xml::xmlKeyRetrigger());
    if (const auto p = parameter(Xml::xmlKeyPitchBendRange().toStdString()); p) {
        m_pitchBendRange = static_cast<int>(p->get().xmlValue());
    }
    m_velocitySensitivity = value(Xml::xmlKeyVelocitySensitivity());
    m_lpfCutoff = value(Xml::xmlKeyLpfCutoff());
    m_hpfCutoff = value(Xml::xmlKeyHpfCutoff());
}

std::vector<std::string> SynthDDevice::modulationPriority() const
{
    return {
        Xml::xmlKeyCutoff().toStdString(),
        Xml::xmlKeyResonance().toStdString(),
        Xml::xmlKeyEnvMod().toStdString(),
        Xml::xmlKeyModDepth().toStdString(),
        Xml::xmlKeyModMix().toStdString(),
        Xml::xmlKeyLfoRate().toStdString(),
        Xml::xmlKeyEnv1Decay().toStdString(),
        Xml::xmlKeyEnv2Decay().toStdString(),
        Xml::xmlKeyGlide().toStdString(),
        Xml::xmlKeyFeedback().toStdString(),
        Xml::xmlKeyVco2Pitch().toStdString(),
        Xml::xmlKeyPanSpread().toStdString(),
        Xml::xmlKeyLpfCutoff().toStdString(),
        Xml::xmlKeyHpfCutoff().toStdString()
    };
}

} // namespace noteahead
