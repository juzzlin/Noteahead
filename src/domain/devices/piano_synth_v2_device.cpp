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

#include "piano_synth_v2_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// Level the summed modes are brought to. The string normalises the strike on its own
// energy, so this is a fixed headroom allowance and nothing more.
constexpr double OutputGain = 0.5;

// What the Release control spans, in seconds of damper decay.
constexpr float MinReleaseTime = 0.02f;
constexpr float ReleaseTimeRange = 0.6f;

} // namespace

void PianoSynthV2Device::Voice::reset()
{
    string.reset();
    active = false;
    pendingRelease = false;
}

PianoSynthV2Device::PianoSynthV2Device(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter(Constants::NahdXml::xmlKeyBrightness().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDecay().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyInharmonicity().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHardness().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyStringDetune().toStdString(), 0.35f, 0, 10000, 3500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyStretch().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyRichness().toStdString(), 0.7f, 0, 10000, 7000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDoubleDecay().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.15f, 0, 10000, 1500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.2f, 0, 10000, 2000, 100));

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    PianoSynthV2Device::syncParameters();
}

PianoSynthV2Device::~PianoSynthV2Device() = default;

std::string PianoSynthV2Device::name() const
{
    return m_name;
}

std::string PianoSynthV2Device::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string PianoSynthV2Device::typeName() const
{
    return Constants::pianoSynthV2DeviceName().toStdString();
}

std::string PianoSynthV2Device::typeIdString()
{
    return "245d8c24-d3ad-43e2-94d6-e3f90c67ab02";
}

std::string PianoSynthV2Device::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> PianoSynthV2Device::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SustainPedal), "Sustain" }
    };
}

void PianoSynthV2Device::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void PianoSynthV2Device::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void PianoSynthV2Device::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
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
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SustainPedal)) {
                const bool pedalOn = value >= 64;
                if (m_sustainPedal && !pedalOn) {
                    for (auto & v : m_voices) {
                        if (v.active && v.pendingRelease) {
                            releaseVoice(v);
                        }
                    }
                }
                m_sustainPedal = pedalOn;
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void PianoSynthV2Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        if (v.active) {
            releaseVoice(v);
        }
    }
    m_sustainPedal = false;
}

void PianoSynthV2Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    m_lpfL.setSampleRate(context.sampleRate);
    m_lpfR.setSampleRate(context.sampleRate);
    m_hpfL.setSampleRate(context.sampleRate);
    m_hpfR.setSampleRate(context.sampleRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));
    m_panner.setPan(static_cast<double>(panInternal()));

    // Strings carry their ringing modes across a rate change themselves, so this must
    // happen once per buffer and outside the per-sample loop.
    for (auto & v : m_voices) {
        v.string.setSampleRate(context.sampleRate);
    }

    const double gain = OutputGain * linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double outL = 0.0;
        double outR = 0.0;

        for (auto & v : m_voices) {
            if (!v.active) {
                continue;
            }

            const double sample = v.string.nextSample() * gain;

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

        m_panner.process(outL, outR);

        outL = m_lpfL.process(outL);
        outR = m_lpfR.process(outR);
        outL = m_hpfL.process(outL);
        outR = m_hpfR.process(outR);

        context.buffer[i * 2] += m_dcBlockerL.process(outL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(outR);
    }
}

bool PianoSynthV2Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & v : m_voices) {
        if (v.active) {
            return true;
        }
    }
    return false;
}

void PianoSynthV2Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void PianoSynthV2Device::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        v.reset();
    }
    m_sustainPedal = false;
    m_nextVoiceToSteal = 0;
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
}

void PianoSynthV2Device::serializeToXml(ProjectWriter & writer) const
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

void PianoSynthV2Device::deserializeFromXml(ProjectReader & reader)
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

ModalPianoString::Settings PianoSynthV2Device::stringSettings() const
{
    return {
        m_brightness,
        m_decay,
        m_inharmonicity,
        m_hammerHardness,
        m_stringDetune,
        m_stretch,
        m_richness,
        m_doubleDecay
    };
}

void PianoSynthV2Device::handleNoteOn(uint8_t note, uint8_t velocity)
{
    int idx = findVoiceForNote(note);
    if (idx < 0) {
        idx = allocateVoice();
    }

    auto & v = m_voices[idx];
    v.string.setSampleRate(sampleRate());
    v.string.trigger(note, static_cast<float>(velocity) / 127.0f, stringSettings());
    v.note = note;
    v.active = true;
    v.pendingRelease = false;
}

void PianoSynthV2Device::handleNoteOff(uint8_t note)
{
    for (auto & v : m_voices) {
        if (v.active && v.note == note) {
            if (m_sustainPedal) {
                v.pendingRelease = true;
            } else {
                releaseVoice(v);
            }
        }
    }
}

void PianoSynthV2Device::releaseVoice(Voice & voice) const
{
    voice.string.release(MinReleaseTime + m_releaseTime * ReleaseTimeRange);
    voice.pendingRelease = false;
}

int PianoSynthV2Device::findVoiceForNote(uint8_t note) const
{
    for (int i = 0; i < MaxVoices; i++) {
        if (m_voices[i].active && m_voices[i].note == note) {
            return i;
        }
    }
    return -1;
}

int PianoSynthV2Device::allocateVoice()
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

float PianoSynthV2Device::noteToPan(uint8_t note) const
{
    // The reference is very nearly mono, so the default spread is slight. The lid still
    // puts the bass to the left of the treble, which is what the spread opens up.
    const float spread = (static_cast<float>(note) - 60.0f) / 60.0f;
    return std::clamp(0.5f + spread * m_stereoWidth * 0.5f, 0.0f, 1.0f);
}

void PianoSynthV2Device::syncParameters()
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
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHardness().toStdString()); p) {
        m_hammerHardness = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringDetune().toStdString()); p) {
        m_stringDetune = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStretch().toStdString()); p) {
        m_stretch = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRichness().toStdString()); p) {
        m_richness = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDoubleDecay().toStdString()); p) {
        m_doubleDecay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p) {
        m_releaseTime = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) {
        m_stereoWidth = p->get().value();
    }
}

float PianoSynthV2Device::brightness() const
{
    return m_brightness;
}

void PianoSynthV2Device::setBrightness(float brightness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyBrightness().toStdString(), brightness);
}

float PianoSynthV2Device::decay() const
{
    return m_decay;
}

void PianoSynthV2Device::setDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDecay().toStdString(), decay);
}

float PianoSynthV2Device::inharmonicity() const
{
    return m_inharmonicity;
}

void PianoSynthV2Device::setInharmonicity(float inharmonicity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyInharmonicity().toStdString(), inharmonicity);
}

float PianoSynthV2Device::hammerHardness() const
{
    return m_hammerHardness;
}

void PianoSynthV2Device::setHammerHardness(float hardness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHardness().toStdString(), hardness);
}

float PianoSynthV2Device::stringDetune() const
{
    return m_stringDetune;
}

void PianoSynthV2Device::setStringDetune(float detune)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringDetune().toStdString(), detune);
}

float PianoSynthV2Device::stretch() const
{
    return m_stretch;
}

void PianoSynthV2Device::setStretch(float stretch)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStretch().toStdString(), stretch);
}

float PianoSynthV2Device::richness() const
{
    return m_richness;
}

void PianoSynthV2Device::setRichness(float richness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyRichness().toStdString(), richness);
}

float PianoSynthV2Device::doubleDecay() const
{
    return m_doubleDecay;
}

void PianoSynthV2Device::setDoubleDecay(float doubleDecay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDoubleDecay().toStdString(), doubleDecay);
}

float PianoSynthV2Device::lpfCutoff() const
{
    return m_lpfCutoff;
}

void PianoSynthV2Device::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float PianoSynthV2Device::hpfCutoff() const
{
    return m_hpfCutoff;
}

void PianoSynthV2Device::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), cutoff);
}

float PianoSynthV2Device::releaseTime() const
{
    return m_releaseTime;
}

void PianoSynthV2Device::setReleaseTime(float releaseTime)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyReleaseTime().toStdString(), releaseTime);
}

float PianoSynthV2Device::stereoWidth() const
{
    return m_stereoWidth;
}

void PianoSynthV2Device::setStereoWidth(float stereoWidth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), stereoWidth);
}

} // namespace noteahead
