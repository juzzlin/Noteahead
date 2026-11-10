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

#include "piano_synth_acoustic_device.hpp"

#include "device_parameter_display.hpp"
#include "piano_synth_acoustic_presets.hpp"
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

//! Brings a note struck at the recording's velocity to about where the recording has it
constexpr double OutputGain = 0.25;

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
        { .controller = static_cast<uint8_t>(Controller::SoundController1), .label = "Hardness", .key = Xml::xmlKeyHardness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Brightness", .key = Xml::xmlKeyBrightness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Release", .key = Xml::xmlKeyReleaseTime().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Decay", .key = Xml::xmlKeyDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Resonance", .key = Xml::xmlKeyResonance().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Knock", .key = Xml::xmlKeyBody().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController8), .label = "Pan Spread", .key = Xml::xmlKeyPanSpread().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController9), .label = "Unison Detune", .key = Xml::xmlKeyStringDetune().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
    };
    return parameters;
}

} // namespace

PianoSynthAcousticDevice::PianoSynthAcousticDevice(std::string name)
  : m_name { std::move(name) }
{
    const auto addKnob = [this](const QString & key, float defaultValue) {
        const auto defaultStep = static_cast<int>(std::lround(defaultValue * 10000.0f));
        addParameter(Parameter(key.toStdString(), defaultValue, 0, 10000, defaultStep, 100));
    };
    // The defaults are the recording: every knob at the value the model was fitted with
    addKnob(Xml::xmlKeyHardness(), 0.5f);
    addKnob(Xml::xmlKeyBody(), 0.5f);
    addKnob(Xml::xmlKeyDecay(), 0.5f);
    addKnob(Xml::xmlKeyBrightness(), 0.5f);
    addKnob(Xml::xmlKeyInharmonicity(), 0.5f);
    addKnob(Xml::xmlKeyStringDetune(), 0.3f);
    addKnob(Xml::xmlKeyReleaseTime(), 0.3f);
    addKnob(Xml::xmlKeyResonance(), 0.5f);
    addKnob(Xml::xmlKeyPanSpread(), 0.6f);
    addKnob(Xml::xmlKeyStretch(), 1.0f);
    addKnob(Xml::xmlKeyVelocitySensitivity(), 0.6f);
    addKnob(Xml::xmlKeyLpfCutoff(), 1.0f);
    addKnob(Xml::xmlKeyHpfCutoff(), 0.0f);

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    PianoSynthAcousticDevice::syncParameters();

    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyHardness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Hardness"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBody(), QT_TRANSLATE_NOOP("ParameterDisplay", "Knock"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBrightness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Brightness"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyInharmonicity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Inharmonicity"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyStringDetune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Unison Detune"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyReleaseTime(), QT_TRANSLATE_NOOP("ParameterDisplay", "Release"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Resonance"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPanSpread(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pan Spread"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyStretch(), QT_TRANSLATE_NOOP("ParameterDisplay", "Stretch"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

PianoSynthAcousticDevice::~PianoSynthAcousticDevice() = default;

std::string PianoSynthAcousticDevice::name() const
{
    return m_name;
}

std::string PianoSynthAcousticDevice::category() const
{
    return Xml::xmlValueSynths().toStdString();
}

std::string PianoSynthAcousticDevice::typeName() const
{
    return Constants::pianoSynthAcousticDeviceName().toStdString();
}

std::string PianoSynthAcousticDevice::typeIdString()
{
    return "c4a7e2d9-5b18-4f63-9e0a-3d2b8f71c6e5";
}

std::string PianoSynthAcousticDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> PianoSynthAcousticDevice::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    std::vector<MidiCcController> controllers {
        { static_cast<uint8_t>(Controller::SustainPedal), "Sustain" },
        { static_cast<uint8_t>(Controller::SoftPedal), "Soft Pedal" }
    };
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

void PianoSynthAcousticDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    // A key struck again while its string still sounds: the string it left is handed to its damper,
    // or to the pedal, and the strike gets a string of its own, so nothing is cut mid-cycle
    if (const int sounding = findVoice(note); sounding >= 0) {
        auto & previous = m_voices.at(static_cast<size_t>(sounding));
        previous.retired = true;
        if (m_sustainPedal) {
            previous.pendingRelease = true;
        } else {
            damp(previous);
        }
    }

    auto & voice = m_voices.at(static_cast<size_t>(allocateVoice()));
    const float v = static_cast<float>(velocity) / 127.0f;
    voice.string.setSampleRate(sampleRate());
    voice.string.strike(note, v, m_stringSettings, m_softPedal);
    voice.active = voice.string.isActive();
    voice.pendingRelease = false;
    voice.retired = false;
    m_soundboard.setSampleRate(sampleRate());
    m_soundboard.knock(note, std::pow(static_cast<double>(v), 1.5));
    m_openStringsDirty = true;
}

void PianoSynthAcousticDevice::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active && !voice.retired && voice.string.note() == note) {
            if (m_sustainPedal) {
                voice.pendingRelease = true;
            } else {
                damp(voice);
            }
        }
    }
    m_openStringsDirty = true;
}

void PianoSynthAcousticDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
            m_softPedal = false;
        } else if (controller == static_cast<uint8_t>(Controller::SustainPedal)) {
            const bool down = value >= 64;
            if (m_sustainPedal && !down) {
                for (auto && voice : m_voices) {
                    if (voice.active && voice.pendingRelease) {
                        damp(voice);
                    }
                }
            }
            m_sustainPedal = down;
            m_openStringsDirty = true;
        } else if (controller == static_cast<uint8_t>(Controller::SoftPedal)) {
            m_softPedal = value >= 64;
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

void PianoSynthAcousticDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_sustainPedal = false;
    for (auto && voice : m_voices) {
        if (voice.active) {
            damp(voice);
        }
    }
    m_openStringsDirty = true;
}

void PianoSynthAcousticDevice::damp(Voice & voice)
{
    voice.string.damp();
    voice.pendingRelease = false;
}

int PianoSynthAcousticDevice::findVoice(uint8_t note) const
{
    for (int i = 0; i < MaxVoices; i++) {
        const auto & voice = m_voices.at(static_cast<size_t>(i));
        if (voice.active && !voice.retired && voice.string.note() == note) {
            return i;
        }
    }
    return -1;
}

int PianoSynthAcousticDevice::allocateVoice()
{
    for (int i = 0; i < MaxVoices; i++) {
        if (!m_voices.at(static_cast<size_t>(i)).active) {
            return i;
        }
    }
    // The quietest string is the one whose loss the new strike covers
    int quietest = 0;
    for (int i = 1; i < MaxVoices; i++) {
        if (m_voices.at(static_cast<size_t>(i)).string.energy() < m_voices.at(static_cast<size_t>(quietest)).string.energy()) {
            quietest = i;
        }
    }
    return quietest;
}

void PianoSynthAcousticDevice::updateOpenStrings()
{
    // With the pedal down every damper is up; without it only the top keys, which have none. A key
    // that is sounding is a string of its own and is left out
    std::bitset<128> open;
    const int from = m_sustainPedal ? PianoSoundboard::LowestNote : ModalGrandString::LowestUndampedNote;
    for (int note = from; note <= PianoSoundboard::HighestNote; note++) {
        open.set(static_cast<size_t>(note));
    }
    for (const auto & voice : m_voices) {
        if (voice.active) {
            open.reset(voice.string.note());
        }
    }
    m_soundboard.setOpenStrings(open);
    m_openStringsDirty = false;
}

void PianoSynthAcousticDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const auto rate = static_cast<double>(context.sampleRate);
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
    m_soundboard.setSampleRate(rate);
    m_soundboard.setTuning(static_cast<double>(m_stringSettings.stretch), std::exp2(2.0 * (static_cast<double>(m_stringSettings.decay) - 0.5)), static_cast<double>(m_stringSettings.panSpread));

    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.string.setSampleRate(rate);
        }
    }
    // A string that has died since the last buffer frees its key for the sympathetic strings
    for (auto && voice : m_voices) {
        if (voice.active && !voice.string.isActive()) {
            voice.active = false;
            m_openStringsDirty = true;
        }
    }
    if (m_openStringsDirty) {
        updateOpenStrings();
    }

    const double gain = OutputGain * linearGainInternal();
    const auto knock = static_cast<double>(m_knock);
    const auto resonance = static_cast<double>(m_resonance);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        for (auto && voice : m_voices) {
            if (voice.active) {
                voice.string.process(left, right);
            }
        }
        m_soundboard.process(0.5 * (left + right), knock, resonance, left, right);

        left *= gain;
        right *= gain;
        m_panner.process(left, right);
        left = m_hpfL.process(m_lpfL.process(left));
        right = m_hpfR.process(m_lpfR.process(right));
        context.buffer[i * 2] += m_dcBlockerL.process(left);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(right);
    }
}

bool PianoSynthAcousticDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const Voice & voice) { return voice.active && voice.string.isActive(); }) || m_soundboard.isActive();
}

void PianoSynthAcousticDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void PianoSynthAcousticDevice::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.string.reset();
        voice.active = false;
        voice.pendingRelease = false;
        voice.retired = false;
    }
    m_sustainPedal = false;
    m_softPedal = false;
    m_soundboard.reset();
    m_openStringsDirty = true;
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void PianoSynthAcousticDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Xml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeStartElement(Xml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Xml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void PianoSynthAcousticDevice::deserializeFromXml(ProjectReader & reader)
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
                } else if (reader.name() == Xml::xmlKeyMidiMappings()) {
                    deserializeMidiMappingsFromXml(reader);
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

void PianoSynthAcousticDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void PianoSynthAcousticDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = PianoSynthAcousticPresets::presets();
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

        for (auto && [name, value] : presets[index].parameters) {
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

float PianoSynthAcousticDevice::value(const QString & key) const
{
    if (const auto p = parameter(key.toStdString()); p) {
        return p->get().value();
    }
    return 0.0f;
}

float PianoSynthAcousticDevice::knob(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return shownParameterValue(key, value(key));
}

void PianoSynthAcousticDevice::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

bool PianoSynthAcousticDevice::sustainPedal() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sustainPedal;
}

int PianoSynthAcousticDevice::activeVoiceCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return static_cast<int>(std::ranges::count_if(m_voices, [](const Voice & voice) { return voice.active; }));
}

void PianoSynthAcousticDevice::syncParameters()
{
    Device::syncParameters();
    m_stringSettings = {
        .hardness = value(Xml::xmlKeyHardness()),
        .decay = value(Xml::xmlKeyDecay()),
        .brightness = value(Xml::xmlKeyBrightness()),
        .inharmonicity = value(Xml::xmlKeyInharmonicity()),
        .detune = value(Xml::xmlKeyStringDetune()),
        .stretch = value(Xml::xmlKeyStretch()),
        .panSpread = value(Xml::xmlKeyPanSpread()),
        .velocitySensitivity = value(Xml::xmlKeyVelocitySensitivity()),
        .release = value(Xml::xmlKeyReleaseTime()),
    };
    m_knock = value(Xml::xmlKeyBody());
    m_resonance = value(Xml::xmlKeyResonance());
    m_lpfCutoff = value(Xml::xmlKeyLpfCutoff());
    m_hpfCutoff = value(Xml::xmlKeyHpfCutoff());
}

std::vector<std::string> PianoSynthAcousticDevice::modulationPriority() const
{
    return {
        Xml::xmlKeyHardness().toStdString(),
        Xml::xmlKeyBrightness().toStdString(),
        Xml::xmlKeyLpfCutoff().toStdString(),
        Xml::xmlKeyResonance().toStdString(),
        Xml::xmlKeyPanSpread().toStdString(),
        Xml::xmlKeyDecay().toStdString(),
        Xml::xmlKeyReleaseTime().toStdString(),
        Xml::xmlKeyBody().toStdString(),
        Xml::xmlKeyStringDetune().toStdString(),
        Xml::xmlKeyHpfCutoff().toStdString()
    };
}

std::vector<std::string> PianoSynthAcousticDevice::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyBrightness().toStdString() };
}

} // namespace noteahead
