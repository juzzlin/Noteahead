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

#include "piano_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

void PianoSynthDevice::Voice::reset()
{
    string.reset();
    active = false;
    pendingRelease = false;
}

PianoSynthDevice::PianoSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter(Constants::NahdXml::xmlKeyBrightness().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDecay().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyInharmonicity().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.7f, 0, 10000, 7000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHardness().toStdString(), 0.5f, 0, 10000, 5000, 100));

    PianoSynthDevice::syncParameters();
}

PianoSynthDevice::~PianoSynthDevice() = default;

std::string PianoSynthDevice::name() const
{
    return m_name;
}

std::string PianoSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string PianoSynthDevice::typeName() const
{
    return Constants::pianoSynthDeviceName().toStdString();
}

std::string PianoSynthDevice::typeIdString()
{
    return "a3f7c2d1-5e8b-4a9f-b1c6-7d0e2f3a4b5c";
}

std::string PianoSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> PianoSynthDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        { static_cast<uint8_t>(Controller::ChannelVolumeMSB), "Volume" },
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SustainPedal), "Sustain" }
    };
}

void PianoSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void PianoSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void PianoSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            m_sustainPedal = false;
            changed = true;
        } else {
            const float val = static_cast<float>(value) / 127.0f;

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SustainPedal)) {
                const bool pedalOn = value >= 64;
                if (m_sustainPedal && !pedalOn) {
                    const float rt = 0.01f + m_releaseTime * 0.49f;
                    for (auto & v : m_voices) {
                        if (v.active && v.pendingRelease) {
                            v.string.release(rt);
                            v.pendingRelease = false;
                        }
                    }
                }
                m_sustainPedal = pedalOn;
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void PianoSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const float rt = 0.01f + m_releaseTime * 0.49f;
    for (auto & v : m_voices) {
        if (v.active) {
            v.string.release(rt);
            v.pendingRelease = false;
        }
    }
    m_sustainPedal = false;
}

void PianoSynthDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    m_panner.setPan(static_cast<double>(panInternal()));

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double outL = 0.0;
        double outR = 0.0;

        for (auto & v : m_voices) {
            if (!v.active) {
                continue;
            }

            v.string.setSampleRate(context.sampleRate);
            const double sample = v.string.nextSample() * static_cast<double>(v.velocity) * linearGainInternal();

            if (!v.string.isActive()) {
                v.active = false;
                continue;
            }

            double voiceL = 0.0;
            double voiceR = 0.0;
            m_voicePanner.setPan(static_cast<double>(noteToPan(v.note)));
            m_voicePanner.processMono(sample, voiceL, voiceR);
            outL += voiceL;
            outR += voiceR;
        }

        outL *= static_cast<double>(volumeInternal());
        outR *= static_cast<double>(volumeInternal());
        m_panner.process(outL, outR);

        context.buffer[i * 2] += m_dcBlockerL.process(outL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(outR);
    }
}

bool PianoSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & v : m_voices) {
        if (v.active) {
            return true;
        }
    }
    return false;
}

void PianoSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void PianoSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        v.reset();
    }
    m_sustainPedal = false;
    m_nextVoiceToSteal = 0;
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void PianoSynthDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void PianoSynthDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
    }
    emit dataChanged();
}

void PianoSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const float vel = static_cast<float>(velocity) / 127.0f;
    const float velBright = vel * m_hammerHardness * 0.5f;
    const float effectiveBright = std::clamp(m_brightness + velBright, 0.0f, 1.0f);

    int idx = findVoiceForNote(note);
    if (idx < 0) {
        idx = allocateVoice();
    }

    auto & v = m_voices[idx];
    v.string.setSampleRate(sampleRate());
    v.string.trigger(note, vel, effectiveBright, m_inharmonicity, m_decay);
    v.note = note;
    v.velocity = vel;
    v.active = true;
    v.pendingRelease = false;
}

void PianoSynthDevice::handleNoteOff(uint8_t note)
{
    const float rt = 0.01f + m_releaseTime * 0.49f;
    for (auto & v : m_voices) {
        if (v.active && v.note == note) {
            if (m_sustainPedal) {
                v.pendingRelease = true;
            } else {
                v.string.release(rt);
            }
        }
    }
}

int PianoSynthDevice::findVoiceForNote(uint8_t note) const
{
    for (int i = 0; i < MaxVoices; i++) {
        if (m_voices[i].active && m_voices[i].note == note) {
            return i;
        }
    }
    return -1;
}

int PianoSynthDevice::allocateVoice()
{
    for (int i = 0; i < MaxVoices; i++) {
        if (!m_voices[i].active) {
            return i;
        }
    }
    const int v = m_nextVoiceToSteal;
    m_nextVoiceToSteal = (m_nextVoiceToSteal + 1) % MaxVoices;
    return v;
}

float PianoSynthDevice::noteToPan(uint8_t note) const
{
    const float spread = (static_cast<float>(note) - 60.0f) / 60.0f;
    return std::clamp(0.5f + spread * m_stereoWidth * 0.5f, 0.0f, 1.0f);
}

void PianoSynthDevice::syncParameters()
{
    Device::syncParameters();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBrightness().toStdString()); p) {
        m_brightness = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        m_decay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyInharmonicity().toStdString()); p) {
        m_inharmonicity = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p) {
        m_releaseTime = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) {
        m_stereoWidth = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHardness().toStdString()); p) {
        m_hammerHardness = p->get().value();
    }
}

float PianoSynthDevice::brightness() const
{
    return m_brightness;
}

void PianoSynthDevice::setBrightness(float brightness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyBrightness().toStdString(), brightness);
}

float PianoSynthDevice::decay() const
{
    return m_decay;
}

void PianoSynthDevice::setDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDecay().toStdString(), decay);
}

float PianoSynthDevice::inharmonicity() const
{
    return m_inharmonicity;
}

void PianoSynthDevice::setInharmonicity(float inharmonicity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyInharmonicity().toStdString(), inharmonicity);
}

float PianoSynthDevice::releaseTime() const
{
    return m_releaseTime;
}

void PianoSynthDevice::setReleaseTime(float releaseTime)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyReleaseTime().toStdString(), releaseTime);
}

float PianoSynthDevice::stereoWidth() const
{
    return m_stereoWidth;
}

void PianoSynthDevice::setStereoWidth(float stereoWidth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), stereoWidth);
}

float PianoSynthDevice::hammerHardness() const
{
    return m_hammerHardness;
}

void PianoSynthDevice::setHammerHardness(float hardness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHardness().toStdString(), hardness);
}

} // namespace noteahead
