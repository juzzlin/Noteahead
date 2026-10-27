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

#include "snare_synth_device.hpp"

#include "device_parameter_display.hpp"
#include "snare_synth_presets.hpp"
#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Keeps the engine's sample rate where its filters and decays stay well behaved.
constexpr double MaxChromaticOctaves = 4.0;

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
    static const std::vector<CcParameter> parameters {
        { .controller = static_cast<uint8_t>(Controller::SoundController1), .label = "Tune", .key = Xml::xmlKeyTune().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Tone", .key = Xml::xmlKeyTone().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Decay", .key = Xml::xmlKeyDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Snappy", .key = Xml::xmlKeySnappy().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Snare Decay", .key = Xml::xmlKeySnareDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Snare Tension", .key = Xml::xmlKeySnareTension().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController8), .label = "Snare Buzz", .key = Xml::xmlKeySnareBuzz().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController9), .label = "Head Tension", .key = Xml::xmlKeyHeadTension().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController10), .label = "Click", .key = Xml::xmlKeyClick().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose1MSB), .label = "Hardness", .key = Xml::xmlKeyHardness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose2MSB), .label = "Position", .key = Xml::xmlKeyPosition().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose3MSB), .label = "Body", .key = Xml::xmlKeyBody().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose4MSB), .label = "Pan Spread", .key = Xml::xmlKeyPanSpread().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose5), .label = "Drive", .key = Xml::xmlKeyDrive().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose7), .label = "Pitch Depth", .key = Xml::xmlKeyPitchDepth().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose8), .label = "Pitch Decay", .key = Xml::xmlKeyPitchDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl2MSB), .label = "Chromatic", .key = Xml::xmlKeyChromaticMode().toStdString(), .isSwitch = true }
    };
    return parameters;
}

} // namespace

SnareSynthDevice::SnareSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    namespace Xml = Constants::NahdXml;
    addParameter(Parameter(Xml::xmlKeyTune().toStdString(), 0.2857f, 0, 10000, 2857, 100));
    addParameter(Parameter(Xml::xmlKeyTone().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyDecay().toStdString(), 0.1554f, 0, 10000, 1554, 100));
    addParameter(Parameter(Xml::xmlKeyHeadTension().toStdString(), 0.35f, 0, 10000, 3500, 100));
    addParameter(Parameter(Xml::xmlKeyPitchDepth().toStdString(), 0.25f, 0, 10000, 2500, 100));
    addParameter(Parameter(Xml::xmlKeyPitchDecay().toStdString(), 0.2105f, 0, 10000, 2105, 100));

    addParameter(Parameter(Xml::xmlKeySnappy().toStdString(), 0.6f, 0, 10000, 6000, 100));
    addParameter(Parameter(Xml::xmlKeySnareDecay().toStdString(), 0.2137f, 0, 10000, 2137, 100));
    addParameter(Parameter(Xml::xmlKeySnareTension().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeySnareBuzz().toStdString(), 0.5f, 0, 10000, 5000, 100));

    addParameter(Parameter(Xml::xmlKeyClick().toStdString(), 0.4f, 0, 10000, 4000, 100));
    addParameter(Parameter(Xml::xmlKeyHardness().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyPosition().toStdString(), 0.2f, 0, 10000, 2000, 100));
    addParameter(Parameter(Xml::xmlKeyBody().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Xml::xmlKeyDrive().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyPanSpread().toStdString(), 0.5f, 0, 10000, 5000, 100));

    addParameter(Parameter { Xml::xmlKeyChromaticMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter(Xml::xmlKeyVelocitySensitivity().toStdString(), 0.7f, 0, 10000, 7000, 100));

    addParameter(Parameter(Xml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Xml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    SnareSynthDevice::syncParameters();

    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tune"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyHeadTension(), QT_TRANSLATE_NOOP("ParameterDisplay", "Head Tension"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPitchDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPitchDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeySnappy(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snappy"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeySnareDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snare Decay"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeySnareTension(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snare Tension"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeySnareBuzz(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snare Buzz"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyClick(), QT_TRANSLATE_NOOP("ParameterDisplay", "Click"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyHardness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Hardness"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPosition(), QT_TRANSLATE_NOOP("ParameterDisplay", "Position"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBody(), QT_TRANSLATE_NOOP("ParameterDisplay", "Body"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDrive(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drive"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPanSpread(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pan Spread"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

SnareSynthDevice::~SnareSynthDevice() = default;

std::string SnareSynthDevice::name() const
{
    return m_name;
}

std::string SnareSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueDrums().toStdString();
}

std::string SnareSynthDevice::typeName() const
{
    return Constants::snareSynthDeviceName().toStdString();
}

std::string SnareSynthDevice::typeIdString()
{
    return "9490a5c3-c239-4631-b36a-441a559f0b10";
}

std::string SnareSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> SnareSynthDevice::deviceMidiCcControllers() const
{
    std::vector<MidiCcController> controllers;
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

float SnareSynthDevice::tuneFrequency() const
{
    return 100.0f + m_tune * 350.0f;
}

bool SnareSynthDevice::supportsOctave() const
{
    return false;
}

double SnareSynthDevice::speedRatio(uint8_t note) const
{
    if (!m_chromatic) {
        return 1.0;
    }
    const double octaves = static_cast<double>(static_cast<int>(note) - RootNote) / 12.0;
    return std::exp2(std::clamp(octaves, -MaxChromaticOctaves, MaxChromaticOctaves));
}

double SnareSynthDevice::engineSampleRate(double sampleRate) const
{
    return sampleRate / m_speedRatio;
}

void SnareSynthDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_speedRatio = speedRatio(note);
    m_engine.setSampleRate(engineSampleRate(static_cast<double>(sampleRate())));
    m_engine.setTune(tuneFrequency());
    const float rawVel = static_cast<float>(velocity) / 127.0f;
    const float vel = (1.0f - m_velocitySensitivity) + m_velocitySensitivity * rawVel;
    m_engine.trigger(vel);
}

void SnareSynthDevice::processDeviceMidiNoteOff(uint8_t)
{
    // One-shot physical model: sound tail is owned by modal decay times.
}

void SnareSynthDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
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

void SnareSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.stop();
}

void SnareSynthDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_engine.setSampleRate(engineSampleRate(static_cast<double>(context.sampleRate)));
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

    const double gain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        float engineL = 0.0f;
        float engineR = 0.0f;
        m_engine.nextSample(engineL, engineR);

        double sampleL = static_cast<double>(engineL) * gain;
        double sampleR = static_cast<double>(engineR) * gain;

        sampleL = m_hpfL.process(m_lpfL.process(sampleL));
        sampleR = m_hpfR.process(m_lpfR.process(sampleR));

        m_panner.process(sampleL, sampleR);

        context.buffer[i * 2] += m_dcBlockerL.process(sampleL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(sampleR);
    }
}

bool SnareSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_engine.isActive();
}

void SnareSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void SnareSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
}

void SnareSynthDevice::serializeToXml(ProjectWriter & writer) const
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

void SnareSynthDevice::deserializeFromXml(ProjectReader & reader)
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
    }
    emit dataChanged();
}

void SnareSynthDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void SnareSynthDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = SnareSynthPresets::presets();
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

void SnareSynthDevice::syncParameters()
{
    Device::syncParameters();

    namespace Xml = Constants::NahdXml;
    if (const auto p = parameter(Xml::xmlKeyTune().toStdString()); p) {
        m_tune = p->get().value();
    }
    if (const auto p = parameter(Xml::xmlKeyTone().toStdString()); p) {
        m_tone = p->get().value();
        m_engine.setTone(m_tone);
    }
    if (const auto p = parameter(Xml::xmlKeyDecay().toStdString()); p) {
        m_decay = p->get().value();
        m_engine.setDecay(0.02f + m_decay * 1.48f);
    }
    if (const auto p = parameter(Xml::xmlKeyHeadTension().toStdString()); p) {
        m_headTension = p->get().value();
        m_engine.setHeadTension(1.0f + m_headTension * 1.0f);
    }
    if (const auto p = parameter(Xml::xmlKeyPitchDepth().toStdString()); p) {
        m_pitchDepth = p->get().value();
        m_engine.setPitchDepth(m_pitchDepth * 24.0f);
    }
    if (const auto p = parameter(Xml::xmlKeyPitchDecay().toStdString()); p) {
        m_pitchDecay = p->get().value();
        m_engine.setPitchDecay(0.005f + m_pitchDecay * 0.095f);
    }
    if (const auto p = parameter(Xml::xmlKeySnappy().toStdString()); p) {
        m_snappy = p->get().value();
        m_engine.setSnappy(m_snappy);
    }
    if (const auto p = parameter(Xml::xmlKeySnareDecay().toStdString()); p) {
        m_snareDecay = p->get().value();
        m_engine.setSnareDecay(0.03f + m_snareDecay * 1.17f);
    }
    if (const auto p = parameter(Xml::xmlKeySnareTension().toStdString()); p) {
        m_snareTension = p->get().value();
        m_engine.setSnareTension(m_snareTension);
    }
    if (const auto p = parameter(Xml::xmlKeySnareBuzz().toStdString()); p) {
        m_snareBuzz = p->get().value();
        m_engine.setSnareBuzz(m_snareBuzz);
    }
    if (const auto p = parameter(Xml::xmlKeyClick().toStdString()); p) {
        m_click = p->get().value();
        m_engine.setClick(m_click);
    }
    if (const auto p = parameter(Xml::xmlKeyHardness().toStdString()); p) {
        m_hardness = p->get().value();
        m_engine.setHardness(m_hardness);
    }
    if (const auto p = parameter(Xml::xmlKeyPosition().toStdString()); p) {
        m_position = p->get().value();
        m_engine.setPosition(m_position);
    }
    if (const auto p = parameter(Xml::xmlKeyBody().toStdString()); p) {
        m_body = p->get().value();
        m_engine.setBody(m_body);
    }
    if (const auto p = parameter(Xml::xmlKeyDrive().toStdString()); p) {
        m_drive = p->get().value();
        m_engine.setDrive(m_drive);
    }
    if (const auto p = parameter(Xml::xmlKeyPanSpread().toStdString()); p) {
        m_panSpread = p->get().value();
        m_engine.setPanSpread(m_panSpread);
    }
    if (const auto p = parameter(Xml::xmlKeyChromaticMode().toStdString()); p) {
        m_chromatic = p->get().value() >= 0.5f;
    }
    if (const auto p = parameter(Xml::xmlKeyVelocitySensitivity().toStdString()); p) {
        m_velocitySensitivity = p->get().value();
    }
    if (const auto p = parameter(Xml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Xml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }
}

float SnareSynthDevice::tune() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyTune(), m_tune);
}

void SnareSynthDevice::setTune(float tune)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyTune().toStdString(), tune);
}

float SnareSynthDevice::tone() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyTone(), m_tone);
}

void SnareSynthDevice::setTone(float tone)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyTone().toStdString(), tone);
}

float SnareSynthDevice::decay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyDecay(), m_decay);
}

void SnareSynthDevice::setDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDecay().toStdString(), decay);
}

float SnareSynthDevice::headTension() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHeadTension(), m_headTension);
}

void SnareSynthDevice::setHeadTension(float ratio)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHeadTension().toStdString(), ratio);
}

float SnareSynthDevice::pitchDepth() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPitchDepth(), m_pitchDepth);
}

void SnareSynthDevice::setPitchDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPitchDepth().toStdString(), depth);
}

float SnareSynthDevice::pitchDecay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPitchDecay(), m_pitchDecay);
}

void SnareSynthDevice::setPitchDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPitchDecay().toStdString(), decay);
}

float SnareSynthDevice::snappy() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeySnappy(), m_snappy);
}

void SnareSynthDevice::setSnappy(float snappy)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySnappy().toStdString(), snappy);
}

float SnareSynthDevice::snareDecay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeySnareDecay(), m_snareDecay);
}

void SnareSynthDevice::setSnareDecay(float decay)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySnareDecay().toStdString(), decay);
}

float SnareSynthDevice::snareTension() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeySnareTension(), m_snareTension);
}

void SnareSynthDevice::setSnareTension(float tension)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySnareTension().toStdString(), tension);
}

float SnareSynthDevice::snareBuzz() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeySnareBuzz(), m_snareBuzz);
}

void SnareSynthDevice::setSnareBuzz(float buzz)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySnareBuzz().toStdString(), buzz);
}

float SnareSynthDevice::click() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyClick(), m_click);
}

void SnareSynthDevice::setClick(float click)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyClick().toStdString(), click);
}

float SnareSynthDevice::hardness() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHardness(), m_hardness);
}

void SnareSynthDevice::setHardness(float hardness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHardness().toStdString(), hardness);
}

float SnareSynthDevice::position() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPosition(), m_position);
}

void SnareSynthDevice::setPosition(float position)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPosition().toStdString(), position);
}

float SnareSynthDevice::body() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyBody(), m_body);
}

void SnareSynthDevice::setBody(float body)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyBody().toStdString(), body);
}

float SnareSynthDevice::drive() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyDrive(), m_drive);
}

void SnareSynthDevice::setDrive(float drive)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDrive().toStdString(), drive);
}

float SnareSynthDevice::panSpread() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPanSpread(), m_panSpread);
}

void SnareSynthDevice::setPanSpread(float spread)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), spread);
}

bool SnareSynthDevice::chromatic() const
{
    return m_chromatic;
}

void SnareSynthDevice::setChromatic(bool chromatic)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyChromaticMode().toStdString(), chromatic ? 1.0f : 0.0f);
}

float SnareSynthDevice::velocitySensitivity() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void SnareSynthDevice::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

float SnareSynthDevice::lpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void SnareSynthDevice::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float SnareSynthDevice::hpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void SnareSynthDevice::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), cutoff);
}

} // namespace noteahead
