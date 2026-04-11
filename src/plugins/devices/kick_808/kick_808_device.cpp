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

#include "kick_808_device.hpp"
#include "kick_808_presets.hpp"

#include "../../../domain/devices/device_parameter_display.hpp"
#include <QtGlobal>

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../common/xml/project_reader.hpp"
#include "../../../common/xml/project_writer.hpp"
#include "../../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>

namespace noteahead {

namespace {

//! Every knob on the panel is reachable over MIDI CC, so a whole kick can be automated from a note
//! column. Keeping the controller numbers, the labels and the parameter keys in one table stops the
//! dispatch below from drifting away from what deviceMidiCcControllers() advertises.
struct CcParameter
{
    uint8_t controller {};
    std::string label;
    std::string key;
    bool isSwitch = false;
};

const std::vector<CcParameter> & ccParameters()
{
    using namespace MidiCcMapping;
    namespace Xml = Constants::NahdXml;
    // LPF and HPF keep the controller numbers the other devices use for them.
    static const std::vector<CcParameter> parameters {
        { .controller = static_cast<uint8_t>(Controller::SoundController1), .label = "Tuning", .key = Xml::xmlKeyTune().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Tone", .key = Xml::xmlKeyTone().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Decay", .key = Xml::xmlKeyDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Pitch Depth", .key = Xml::xmlKeyPitchDepth().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Pitch Decay", .key = Xml::xmlKeyPitchDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Key Track", .key = Xml::xmlKeyKeyTrack().toStdString(), .isSwitch = true },
        { .controller = static_cast<uint8_t>(Controller::PortamentoTimeMSB), .label = "Glide", .key = Xml::xmlKeyPortamento().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose5), .label = "Drive", .key = Xml::xmlKeyDrive().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() }
    };
    return parameters;
}

namespace Xml = Constants::NahdXml;

//! Zero is no release stage at all, as DrumAmpEnvelope reads it: the kick rings out
double releaseSeconds(float position)
{
    return position > 0.0f ? ParameterMapper::mapExponential(position, DrumAmpEnvelope::KnobMinDecaySeconds, DrumAmpEnvelope::KnobMaxDecaySeconds) : 0.0;
}

} // namespace

Kick808Device::Kick808Device(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter(Constants::NahdXml::xmlKeyTune().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyTone().toStdString(), 0.35f, 0, 10000, 3500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDecay().toStdString(), 0.6f, 0, 10000, 6000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.25f, 0, 10000, 2500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDrive().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyTrack().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    // The amp envelope, at defaults that leave the kick as it is
    addParameter(Parameter(Xml::xmlKeyAmpAttack().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyAmpHold().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyAmpDecay().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Xml::xmlKeyAmpRelease().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyAmpCurve().toStdString(), 0.0f, 0, 10000, 0, 100));

    m_lpf.setMode(CascadedSvf::Mode::LowPass);
    m_hpf.setMode(CascadedSvf::Mode::HighPass);

    Kick808Device::syncParameters();

    // What a parameter automation may drive, and how it reads out: the controls worth riding, in
    // the units their knobs show.
    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tuning"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Decay"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyDrive(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drive"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyPitchDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyPitchDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Decay"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyPortamento(), QT_TRANSLATE_NOOP("ParameterDisplay", "Glide"));
    setParameterDisplay(Xml::xmlKeyAmpAttack().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = DrumAmpEnvelope::KnobMaxAttackSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Attack") });
    setParameterDisplay(Xml::xmlKeyAmpHold().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = DrumAmpEnvelope::KnobMaxHoldSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Hold") });
    setParameterDisplay(Xml::xmlKeyAmpDecay().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = DrumAmpEnvelope::KnobMinDecaySeconds, .max = DrumAmpEnvelope::KnobMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Decay") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpSustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Sustain"));
    setParameterDisplay(Xml::xmlKeyAmpRelease().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = DrumAmpEnvelope::KnobMinDecaySeconds, .max = DrumAmpEnvelope::KnobMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Release") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpCurve(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Curve"));
}

Kick808Device::~Kick808Device() = default;

std::string Kick808Device::name() const
{
    return m_name;
}

std::string Kick808Device::category() const
{
    return Constants::NahdXml::xmlValueDrums().toStdString();
}

std::string Kick808Device::typeName() const
{
    return Constants::kick808DeviceName().toStdString();
}

std::string Kick808Device::typeIdString()
{
    return "b8c4e1a7-2d63-4f95-8a10-3e7b6c9d0f42";
}

std::string Kick808Device::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> Kick808Device::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    std::vector<MidiCcController> controllers {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" }
    };
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

void Kick808Device::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.setSampleRate(sampleRate());
    // Off, Key Track pins the kick where the hardware's single tuned voice sits
    m_engine.setNote(m_keyTrack ? static_cast<float>(note) : Kick808Engine::ReferenceNote);
    m_elapsedFrames = 0;
    const float vel = static_cast<float>(velocity) / 127.0f;
    if (m_ampEnvelopeEnabled) {
        m_strike.strike(m_engine, m_ampEnvelope, vel);
    } else {
        m_engine.trigger(vel);
    }
}

void Kick808Device::processDeviceMidiNoteOff(uint8_t)
{
    // One-shot by design: the tail is owned by Decay, not by note length. Only the amp envelope's
    // release has anything to do with a note ending.
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (m_ampEnvelopeEnabled) {
        m_ampEnvelope.release();
    }
}

void Kick808Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
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
                        p->get().setAutomationValue(it->isSwitch ? (value >= 64 ? 1.0f : 0.0f) : val);
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

void Kick808Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.stop();
}

void Kick808Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_engine.setSampleRate(context.sampleRate);
    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    m_lpf.setSampleRate(context.sampleRate);
    m_hpf.setSampleRate(context.sampleRate);
    m_lpf.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpf.setCutoff(static_cast<double>(m_hpfCutoff));
    m_panner.setPan(static_cast<double>(panInternal()));
    m_ampEnvelope.setSampleRate(static_cast<double>(context.sampleRate));

    const double gain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        if (m_ampEnvelopeEnabled) {
            m_strike.advance(m_engine, m_ampEnvelope);
        }
        const double amp = m_ampEnvelopeEnabled ? m_ampEnvelope.nextSample() : 1.0;
        double sample = static_cast<double>(m_engine.nextSample()) * gain * amp;
        sample = m_hpf.process(m_lpf.process(sample));

        double outL = 0.0;
        double outR = 0.0;
        m_panner.processMono(sample, outL, outR);

        context.buffer[i * 2] += m_dcBlockerL.process(outL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(outR);
    }
    m_elapsedFrames += context.frameCount;
}

bool Kick808Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_engine.isActive() && (!m_ampEnvelopeEnabled || m_ampEnvelope.isActive());
}

std::optional<double> Kick808Device::elapsedSeconds() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (!hasActiveAudio() || sampleRate() == 0) {
        return {};
    }
    return static_cast<double>(m_elapsedFrames) / sampleRate();
}

void Kick808Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void Kick808Device::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.reset();
    m_ampEnvelope.reset();
    m_strike.reset();
    m_lpf.reset();
    m_hpf.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void Kick808Device::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void Kick808Device::deserializeFromXml(ProjectReader & reader)
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
                } else if (reader.name() == Constants::NahdXml::xmlKeyMidiMappings()) {
                    deserializeMidiMappingsFromXml(reader);
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
    }
    emit dataChanged();
}

void Kick808Device::syncParameters()
{
    Device::syncParameters();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyTune().toStdString()); p) {
        m_tune = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
        m_tone = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        m_decay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p) {
        m_pitchDepth = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p) {
        m_pitchDecay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDrive().toStdString()); p) {
        m_drive = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) {
        m_glide = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) {
        m_keyTrack = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }

    m_engine.setTune(m_tune);
    m_engine.setTone(m_tone);
    m_engine.setDecay(m_decay);
    m_engine.setPitchDepth(m_pitchDepth);
    m_engine.setPitchDecay(m_pitchDecay);
    m_engine.setDrive(m_drive);
    m_engine.setGlide(m_glide);

    syncAmpEnvelope();
}

void Kick808Device::syncAmpEnvelope()
{
    m_ampEnvelope.setAttackTime(ampAttackSeconds());
    m_ampEnvelope.setHoldTime(ampHoldSeconds());
    m_ampEnvelope.setDecayTime(ampDecaySeconds());
    const auto sustain = value(Xml::xmlKeyAmpSustain());
    m_ampEnvelope.setSustainLevel(static_cast<double>(sustain));
    m_ampEnvelope.setReleaseTime(ampReleaseSeconds());
    m_ampEnvelope.setCurve(static_cast<double>(value(Xml::xmlKeyAmpCurve())));

    const bool enabled = value(Xml::xmlKeyAmpAttack()) > 0.0f || sustain < 1.0f || value(Xml::xmlKeyAmpRelease()) > 0.0f;
    if (enabled && !m_ampEnvelopeEnabled && m_engine.isActive()) {
        // Turned on under a ringing kick: the envelope picks it up rather than cutting it
        m_ampEnvelope.trigger();
    }
    if (!enabled && m_ampEnvelopeEnabled) {
        // Switched off during a retrigger's fade: the hit it held back is struck now
        m_strike.strikeNow(m_engine);
    }
    m_ampEnvelopeEnabled = enabled;
}

float Kick808Device::value(const QString & key) const
{
    const auto p = parameter(key.toStdString());
    return p ? p->get().value() : 0.0f;
}

float Kick808Device::knob(const QString & key) const
{
    return shownParameterValue(key, value(key));
}

void Kick808Device::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

double Kick808Device::ampAttackSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpAttack()), 0.0, DrumAmpEnvelope::KnobMaxAttackSeconds);
}

double Kick808Device::ampHoldSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpHold()), 0.0, DrumAmpEnvelope::KnobMaxHoldSeconds);
}

double Kick808Device::ampDecaySeconds() const
{
    return ParameterMapper::mapExponential(value(Xml::xmlKeyAmpDecay()), DrumAmpEnvelope::KnobMinDecaySeconds, DrumAmpEnvelope::KnobMaxDecaySeconds);
}

double Kick808Device::ampReleaseSeconds() const
{
    return releaseSeconds(value(Xml::xmlKeyAmpRelease()));
}

bool Kick808Device::ampEnvelopeEnabled() const
{
    return m_ampEnvelopeEnabled;
}

void Kick808Device::loadPreset(int index)
{
    applyPreset(index, true);
}

void Kick808Device::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = Kick808Presets::presets();
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

float Kick808Device::tune() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyTune(), m_tune);
}

void Kick808Device::setTune(float tune)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyTune().toStdString(), tune);
}

float Kick808Device::tone() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyTone(), m_tone);
}

void Kick808Device::setTone(float tone)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyTone().toStdString(), tone);
}

float Kick808Device::decay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyDecay(), m_decay);
}

void Kick808Device::setDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDecay().toStdString(), decay);
}

float Kick808Device::pitchDepth() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPitchDepth(), m_pitchDepth);
}

void Kick808Device::setPitchDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPitchDepth().toStdString(), depth);
}

float Kick808Device::pitchDecay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPitchDecay(), m_pitchDecay);
}

void Kick808Device::setPitchDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPitchDecay().toStdString(), decay);
}

float Kick808Device::drive() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyDrive(), m_drive);
}

void Kick808Device::setDrive(float drive)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDrive().toStdString(), drive);
}

float Kick808Device::glide() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPortamento(), m_glide);
}

void Kick808Device::setGlide(float glide)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPortamento().toStdString(), glide);
}

bool Kick808Device::keyTrack() const
{
    return m_keyTrack;
}

void Kick808Device::setKeyTrack(bool keyTrack)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyKeyTrack().toStdString(), keyTrack ? 1.0f : 0.0f);
}

float Kick808Device::lpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void Kick808Device::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float Kick808Device::hpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void Kick808Device::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), cutoff);
}

std::vector<std::string> Kick808Device::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyTone().toStdString(),
        NahdXml::xmlKeyDecay().toStdString(),
        NahdXml::xmlKeyPitchDepth().toStdString(),
        NahdXml::xmlKeyPitchDecay().toStdString(),
        NahdXml::xmlKeyDrive().toStdString(),
        NahdXml::xmlKeyTune().toStdString(),
        NahdXml::xmlKeyLpfCutoff().toStdString(),
        NahdXml::xmlKeyHpfCutoff().toStdString()
    };
}

std::vector<std::string> Kick808Device::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyTone().toStdString() };
}

} // namespace noteahead
