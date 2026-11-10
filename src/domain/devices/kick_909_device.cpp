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

#include "kick_909_device.hpp"

#include "device_parameter_display.hpp"
#include "kick_909_presets.hpp"
#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

//! How far a note can move Pitch either way in Chromatic mode
constexpr double MaxChromaticOctaves = 3.0;

//! Puts Init's peak at about -5 dBFS, where the Kick Synth's is, after the centre pan's -3 dB
constexpr double OutputGain = 0.662;

//! The amp envelope's ranges, the drum machines' own
constexpr double AmpMaxAttackSeconds = 1.0;
constexpr double AmpMaxHoldSeconds = 8.0;
constexpr double AmpMinDecaySeconds = 0.005;
constexpr double AmpMaxDecaySeconds = 8.0;

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
    static const std::vector<CcParameter> parameters {
        { .controller = static_cast<uint8_t>(Controller::SoundController1), .label = "Tune", .key = Xml::xmlKeyTune().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Attack", .key = Xml::xmlKeyAttack().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Decay", .key = Xml::xmlKeyDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Pitch", .key = Xml::xmlKeyPitch().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Pitch Depth", .key = Xml::xmlKeyPitchDepth().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose5), .label = "Drive", .key = Xml::xmlKeyDrive().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl2MSB), .label = "Chromatic", .key = Xml::xmlKeyChromaticMode().toStdString(), .isSwitch = true }
    };
    return parameters;
}

} // namespace

Kick909Device::Kick909Device(std::string name)
  : m_name { std::move(name) }
{
    const auto addKnob = [this](const QString & key, float defaultValue) {
        const auto defaultStep = static_cast<int>(std::lround(defaultValue * 10000.0f));
        addParameter(Parameter(key.toStdString(), defaultValue, 0, 10000, defaultStep, 100));
    };
    // The defaults are the 909's with every knob at noon, and the RD-9's extras where the 909 is
    addKnob(Xml::xmlKeyTune(), 0.5f);
    addKnob(Xml::xmlKeyAttack(), 0.5f);
    addKnob(Xml::xmlKeyDecay(), 0.5f);
    addKnob(Xml::xmlKeyPitch(), 0.5f);
    addKnob(Xml::xmlKeyPitchDepth(), 0.5f);
    addKnob(Xml::xmlKeyDrive(), 0.0f);

    addKnob(Xml::xmlKeyAmpAttack(), 0.0f);
    addKnob(Xml::xmlKeyAmpHold(), 0.0f);
    addKnob(Xml::xmlKeyAmpDecay(), 0.0f);
    addKnob(Xml::xmlKeyAmpSustain(), 1.0f);
    addKnob(Xml::xmlKeyAmpRelease(), 0.0f);
    addKnob(Xml::xmlKeyAmpCurve(), 0.0f);

    addParameter(Parameter { Xml::xmlKeyChromaticMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addKnob(Xml::xmlKeyVelocitySensitivity(), 0.7f);
    addKnob(Xml::xmlKeyLpfCutoff(), 1.0f);
    addKnob(Xml::xmlKeyHpfCutoff(), 0.0f);

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    Kick909Device::syncParameters();

    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tune"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Attack"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPitch(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPitchDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDrive(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drive"));
    setParameterDisplay(Xml::xmlKeyAmpAttack().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = AmpMaxAttackSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Attack") });
    setParameterDisplay(Xml::xmlKeyAmpHold().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = AmpMaxHoldSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Hold") });
    setParameterDisplay(Xml::xmlKeyAmpDecay().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = AmpMinDecaySeconds, .max = AmpMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Decay") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpSustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Sustain"));
    setParameterDisplay(Xml::xmlKeyAmpRelease().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = AmpMinDecaySeconds, .max = AmpMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Release") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpCurve(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Curve"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

Kick909Device::~Kick909Device() = default;

std::string Kick909Device::name() const
{
    return m_name;
}

std::string Kick909Device::category() const
{
    return Xml::xmlValueDrums().toStdString();
}

std::string Kick909Device::typeName() const
{
    return Constants::kick909DeviceName().toStdString();
}

std::string Kick909Device::typeIdString()
{
    return "b4e1c9d0-3a7f-4e25-9c86-d05f2a7e1b93";
}

std::string Kick909Device::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> Kick909Device::deviceMidiCcControllers() const
{
    std::vector<MidiCcController> controllers;
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

bool Kick909Device::supportsOctave() const
{
    return false;
}

double Kick909Device::noteRatio(uint8_t note) const
{
    if (!m_chromatic) {
        return 1.0;
    }
    const double octaves = static_cast<double>(static_cast<int>(note) - RootNote) / 12.0;
    return std::exp2(std::clamp(octaves, -MaxChromaticOctaves, MaxChromaticOctaves));
}

void Kick909Device::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_note = note;
    m_engine.setNoteRatio(noteRatio(note));
    const float rawVel = static_cast<float>(velocity) / 127.0f;
    const float vel = (1.0f - m_velocitySensitivity) + m_velocitySensitivity * rawVel;
    m_engine.trigger(vel);
    if (m_ampEnvelopeEnabled) {
        m_ampEnvelope.trigger();
    }
}

void Kick909Device::processDeviceMidiNoteOff(uint8_t)
{
    // The drum's own tail is its Decay; only the amp envelope's release has anything to do
    // with a note ending
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (m_ampEnvelopeEnabled) {
        m_ampEnvelope.release();
    }
}

void Kick909Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
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

void Kick909Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.stop();
}

void Kick909Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_engine.setSampleRate(static_cast<double>(context.sampleRate));
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
    m_ampEnvelope.setSampleRate(static_cast<double>(context.sampleRate));

    const double gain = OutputGain * linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const double amp = m_ampEnvelopeEnabled ? m_ampEnvelope.nextSample() : 1.0;
        double sampleL = static_cast<double>(m_engine.nextSample()) * gain * amp;
        double sampleR = sampleL;

        sampleL = m_hpfL.process(m_lpfL.process(sampleL));
        sampleR = m_hpfR.process(m_lpfR.process(sampleR));

        m_panner.process(sampleL, sampleR);

        context.buffer[i * 2] += m_dcBlockerL.process(sampleL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(sampleR);
    }
}

bool Kick909Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_engine.isActive() && (!m_ampEnvelopeEnabled || m_ampEnvelope.isActive());
}

void Kick909Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void Kick909Device::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.reset();
    m_ampEnvelope.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void Kick909Device::serializeToXml(ProjectWriter & writer) const
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

void Kick909Device::deserializeFromXml(ProjectReader & reader)
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

void Kick909Device::loadPreset(int index)
{
    applyPreset(index, true);
}

void Kick909Device::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = Kick909Presets::presets();
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

float Kick909Device::value(const QString & key) const
{
    if (const auto p = parameter(key.toStdString()); p) {
        return p->get().value();
    }
    return 0.0f;
}

float Kick909Device::knob(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return shownParameterValue(key, value(key));
}

void Kick909Device::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

void Kick909Device::syncParameters()
{
    Device::syncParameters();
    m_chromatic = value(Xml::xmlKeyChromaticMode()) >= 0.5f;
    syncEngine();
    syncAmpEnvelope();
    m_velocitySensitivity = value(Xml::xmlKeyVelocitySensitivity());
    m_lpfCutoff = value(Xml::xmlKeyLpfCutoff());
    m_hpfCutoff = value(Xml::xmlKeyHpfCutoff());
}

void Kick909Device::syncEngine()
{
    m_engine.setTune(value(Xml::xmlKeyTune()));
    m_engine.setAttack(value(Xml::xmlKeyAttack()));
    m_engine.setDecay(value(Xml::xmlKeyDecay()));
    m_engine.setPitch(value(Xml::xmlKeyPitch()));
    m_engine.setPitchDepth(value(Xml::xmlKeyPitchDepth()));
    m_engine.setNoteRatio(noteRatio(m_note));
    m_engine.setDrive(value(Xml::xmlKeyDrive()));
}

void Kick909Device::syncAmpEnvelope()
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
        // Turned on under a ringing drum: the envelope picks it up at full rather than cutting it
        m_ampEnvelope.trigger();
    }
    m_ampEnvelopeEnabled = enabled;
}

bool Kick909Device::ampEnvelopeEnabled() const
{
    return m_ampEnvelopeEnabled;
}

double Kick909Device::ampAttackSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpAttack()), 0.0, AmpMaxAttackSeconds);
}

double Kick909Device::ampHoldSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpHold()), 0.0, AmpMaxHoldSeconds);
}

double Kick909Device::ampDecaySeconds() const
{
    return ParameterMapper::mapExponential(value(Xml::xmlKeyAmpDecay()), AmpMinDecaySeconds, AmpMaxDecaySeconds);
}

double Kick909Device::ampReleaseSeconds() const
{
    // Zero is no release stage at all, as DrumAmpEnvelope reads it: the drum rings out
    const auto release = value(Xml::xmlKeyAmpRelease());
    return release > 0.0f ? ParameterMapper::mapExponential(release, AmpMinDecaySeconds, AmpMaxDecaySeconds) : 0.0;
}

bool Kick909Device::chromatic() const
{
    return m_chromatic;
}

void Kick909Device::setChromatic(bool chromatic)
{
    setContinuousParameterValue(Xml::xmlKeyChromaticMode().toStdString(), chromatic ? 1.0f : 0.0f);
}

std::vector<std::string> Kick909Device::modulationPriority() const
{
    return {
        Xml::xmlKeyDecay().toStdString(),
        Xml::xmlKeyTune().toStdString(),
        Xml::xmlKeyPitch().toStdString(),
        Xml::xmlKeyPitchDepth().toStdString(),
        Xml::xmlKeyAttack().toStdString(),
        Xml::xmlKeyDrive().toStdString(),
        Xml::xmlKeyLpfCutoff().toStdString(),
        Xml::xmlKeyHpfCutoff().toStdString()
    };
}

std::vector<std::string> Kick909Device::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyDecay().toStdString() };
}

} // namespace noteahead
