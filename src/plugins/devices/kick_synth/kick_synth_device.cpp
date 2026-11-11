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

#include "kick_synth_device.hpp"

#include "../../../domain/devices/device_parameter_display.hpp"
#include "kick_synth_presets.hpp"
#include <QtGlobal>

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../common/xml/project_reader.hpp"
#include "../../../common/xml/project_writer.hpp"
#include "../../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

//! Keeps the engine's sample rate where its filters and decays stay well behaved.
constexpr double MaxChromaticOctaves = 4.0;

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
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Tone", .key = Xml::xmlKeyTone().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Decay", .key = Xml::xmlKeyDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Damping", .key = Xml::xmlKeyDamping().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Pitch Drop", .key = Xml::xmlKeyPitchDrop().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Beater", .key = Xml::xmlKeyHardness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController8), .label = "Click", .key = Xml::xmlKeyClick().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController9), .label = "Front Head", .key = Xml::xmlKeyFrontHead().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController10), .label = "Front Tune", .key = Xml::xmlKeyFrontTune().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose1MSB), .label = "Ring", .key = Xml::xmlKeyRing().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose2MSB), .label = "Position", .key = Xml::xmlKeyPosition().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose3MSB), .label = "Shell", .key = Xml::xmlKeyShell().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose4MSB), .label = "Mic", .key = Xml::xmlKeyMicPosition().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose5), .label = "Drive", .key = Xml::xmlKeyDrive().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose7), .label = "Ring Decay", .key = Xml::xmlKeyRingDecay().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl2MSB), .label = "Chromatic", .key = Xml::xmlKeyChromaticMode().toStdString(), .isSwitch = true }
    };
    return parameters;
}

//! The knob position of @p value on an exponential sweep from @p min to @p max
float exponentialKnob(double value, double min, double max)
{
    return static_cast<float>(std::log(value / min) / std::log(max / min));
}

} // namespace

KickSynthDevice::KickSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    const auto addKnob = [this](const QString & key, float defaultValue) {
        const auto defaultStep = static_cast<int>(std::lround(defaultValue * 10000.0f));
        addParameter(Parameter(key.toStdString(), defaultValue, 0, 10000, defaultStep, 100));
    };
    // The defaults are a middle-of-the-road kit drum: a 22" at 55 Hz, a felt beater, a pillow and a
    // solid front head tuned a little above the batter
    addKnob(Xml::xmlKeyTune(), exponentialKnob(55.0, MinTuneHz, MaxTuneHz));
    addKnob(Xml::xmlKeyDecay(), exponentialKnob(1.5, MinDecaySeconds, MaxDecaySeconds));
    addKnob(Xml::xmlKeyTone(), 0.4f);
    addKnob(Xml::xmlKeyPitchDrop(), 0.6f);
    addKnob(Xml::xmlKeyDamping(), 0.6f);
    addKnob(Xml::xmlKeyRing(), 0.35f);
    addKnob(Xml::xmlKeyRingDecay(), exponentialKnob(0.4, MinRingDecaySeconds, MaxRingDecaySeconds));

    addKnob(Xml::xmlKeyHardness(), 0.3f);
    addKnob(Xml::xmlKeyClick(), 0.3f);
    addKnob(Xml::xmlKeyPosition(), 0.3f);

    addKnob(Xml::xmlKeyFrontHead(), 0.6f);
    addKnob(Xml::xmlKeyFrontTune(), exponentialKnob(1.6, MinFrontTune, MaxFrontTune));
    addKnob(Xml::xmlKeyShell(), 0.5f);
    addKnob(Xml::xmlKeyMicPosition(), 0.5f);
    addKnob(Xml::xmlKeyDrive(), 0.3f);

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

    KickSynthDevice::syncParameters();

    DeviceParameterDisplay::filters(*this);
    setParameterDisplay(Xml::xmlKeyTune().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "Hz", .min = MinTuneHz, .max = MaxTuneHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Tune") });
    setParameterDisplay(Xml::xmlKeyDecay().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = MinDecaySeconds, .max = MaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Decay") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPitchDrop(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Drop"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDamping(), QT_TRANSLATE_NOOP("ParameterDisplay", "Damping"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyRing(), QT_TRANSLATE_NOOP("ParameterDisplay", "Ring"));
    setParameterDisplay(Xml::xmlKeyRingDecay().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = MinRingDecaySeconds, .max = MaxRingDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Ring Decay") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyHardness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Beater"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyClick(), QT_TRANSLATE_NOOP("ParameterDisplay", "Click"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPosition(), QT_TRANSLATE_NOOP("ParameterDisplay", "Position"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyFrontHead(), QT_TRANSLATE_NOOP("ParameterDisplay", "Front Head"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyFrontTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Front Tune"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyShell(), QT_TRANSLATE_NOOP("ParameterDisplay", "Shell"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyMicPosition(), QT_TRANSLATE_NOOP("ParameterDisplay", "Mic"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyDrive(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drive"));
    setParameterDisplay(Xml::xmlKeyAmpAttack().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = AmpMaxAttackSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Attack") });
    setParameterDisplay(Xml::xmlKeyAmpHold().toStdString(), { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = AmpMaxHoldSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Hold") });
    setParameterDisplay(Xml::xmlKeyAmpDecay().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = AmpMinDecaySeconds, .max = AmpMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Decay") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpSustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Sustain"));
    setParameterDisplay(Xml::xmlKeyAmpRelease().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = AmpMinDecaySeconds, .max = AmpMaxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Release") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyAmpCurve(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Curve"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

KickSynthDevice::~KickSynthDevice() = default;

std::string KickSynthDevice::name() const
{
    return m_name;
}

std::string KickSynthDevice::category() const
{
    return Xml::xmlValueDrums().toStdString();
}

std::string KickSynthDevice::typeName() const
{
    return Constants::kickSynthDeviceName().toStdString();
}

std::string KickSynthDevice::typeIdString()
{
    return "5d0c7e2a-9b41-4f6e-a3d8-1c72e5b09f34";
}

std::string KickSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> KickSynthDevice::deviceMidiCcControllers() const
{
    std::vector<MidiCcController> controllers;
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

double KickSynthDevice::tuneHz(float knob)
{
    return ParameterMapper::mapExponential(knob, MinTuneHz, MaxTuneHz);
}

double KickSynthDevice::decaySeconds(float knob)
{
    return ParameterMapper::mapExponential(knob, MinDecaySeconds, MaxDecaySeconds);
}

double KickSynthDevice::frontTuneRatio(float knob)
{
    return ParameterMapper::mapExponential(knob, MinFrontTune, MaxFrontTune);
}

double KickSynthDevice::ringLevel(float knob)
{
    // Squared, so the low end where most kicks sit is not crowded into the first few degrees
    return MaxRing * static_cast<double>(knob) * static_cast<double>(knob);
}

double KickSynthDevice::ringDecaySeconds(float knob)
{
    return ParameterMapper::mapExponential(knob, MinRingDecaySeconds, MaxRingDecaySeconds);
}

bool KickSynthDevice::supportsOctave() const
{
    return false;
}

double KickSynthDevice::speedRatio(uint8_t note) const
{
    if (!m_chromatic) {
        return 1.0;
    }
    const double octaves = static_cast<double>(static_cast<int>(note) - RootNote) / 12.0;
    return std::exp2(std::clamp(octaves, -MaxChromaticOctaves, MaxChromaticOctaves));
}

double KickSynthDevice::engineSampleRate(double sampleRate) const
{
    return sampleRate / m_speedRatio;
}

void KickSynthDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_speedRatio = speedRatio(note);
    m_engine.setSampleRate(engineSampleRate(static_cast<double>(sampleRate())));
    const float rawVel = static_cast<float>(velocity) / 127.0f;
    const float vel = (1.0f - m_velocitySensitivity) + m_velocitySensitivity * rawVel;
    m_engine.trigger(vel);
    if (m_ampEnvelopeEnabled) {
        m_ampEnvelope.trigger();
    }
}

void KickSynthDevice::processDeviceMidiNoteOff(uint8_t)
{
    // The drum's own tail is its modal decays; only the amp envelope's release has anything to do
    // with a note ending
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (m_ampEnvelopeEnabled) {
        m_ampEnvelope.release();
    }
}

void KickSynthDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
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

void KickSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_engine.stop();
}

void KickSynthDevice::processAudio(AudioContext & context)
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
    m_ampEnvelope.setSampleRate(static_cast<double>(context.sampleRate));

    const double gain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        float engineL = 0.0f;
        float engineR = 0.0f;
        m_engine.nextSample(engineL, engineR);

        const double amp = m_ampEnvelopeEnabled ? m_ampEnvelope.nextSample() : 1.0;
        double sampleL = static_cast<double>(engineL) * gain * amp;
        double sampleR = static_cast<double>(engineR) * gain * amp;

        sampleL = m_hpfL.process(m_lpfL.process(sampleL));
        sampleR = m_hpfR.process(m_lpfR.process(sampleR));

        m_panner.process(sampleL, sampleR);

        context.buffer[i * 2] += m_dcBlockerL.process(sampleL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(sampleR);
    }
}

bool KickSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_engine.isActive() && (!m_ampEnvelopeEnabled || m_ampEnvelope.isActive());
}

void KickSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void KickSynthDevice::resetDeviceAudio()
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

void KickSynthDevice::serializeToXml(ProjectWriter & writer) const
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

void KickSynthDevice::deserializeFromXml(ProjectReader & reader)
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

void KickSynthDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void KickSynthDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = KickSynthPresets::presets();
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

float KickSynthDevice::value(const QString & key) const
{
    if (const auto p = parameter(key.toStdString()); p) {
        return p->get().value();
    }
    return 0.0f;
}

float KickSynthDevice::knob(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return shownParameterValue(key, value(key));
}

void KickSynthDevice::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

void KickSynthDevice::syncParameters()
{
    Device::syncParameters();
    syncEngine();
    syncAmpEnvelope();
    m_chromatic = value(Xml::xmlKeyChromaticMode()) >= 0.5f;
    m_velocitySensitivity = value(Xml::xmlKeyVelocitySensitivity());
    m_lpfCutoff = value(Xml::xmlKeyLpfCutoff());
    m_hpfCutoff = value(Xml::xmlKeyHpfCutoff());
}

void KickSynthDevice::syncEngine()
{
    m_engine.setTune(static_cast<float>(tuneHz(value(Xml::xmlKeyTune()))));
    m_engine.setDecay(static_cast<float>(decaySeconds(value(Xml::xmlKeyDecay()))));
    m_engine.setTone(value(Xml::xmlKeyTone()));
    m_engine.setPitchDrop(value(Xml::xmlKeyPitchDrop()));
    m_engine.setDamping(value(Xml::xmlKeyDamping()));
    m_engine.setRing(static_cast<float>(ringLevel(value(Xml::xmlKeyRing()))));
    m_engine.setRingDecay(static_cast<float>(ringDecaySeconds(value(Xml::xmlKeyRingDecay()))));
    m_engine.setBeater(value(Xml::xmlKeyHardness()));
    m_engine.setClick(value(Xml::xmlKeyClick()));
    m_engine.setPosition(value(Xml::xmlKeyPosition()));
    m_engine.setFrontHead(value(Xml::xmlKeyFrontHead()));
    m_engine.setFrontTune(static_cast<float>(frontTuneRatio(value(Xml::xmlKeyFrontTune()))));
    m_engine.setShell(value(Xml::xmlKeyShell()));
    m_engine.setMic(value(Xml::xmlKeyMicPosition()));
    m_engine.setDrive(value(Xml::xmlKeyDrive()));
}

void KickSynthDevice::syncAmpEnvelope()
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

bool KickSynthDevice::ampEnvelopeEnabled() const
{
    return m_ampEnvelopeEnabled;
}

double KickSynthDevice::ampAttackSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpAttack()), 0.0, AmpMaxAttackSeconds);
}

double KickSynthDevice::ampHoldSeconds() const
{
    return ParameterMapper::mapCubic(value(Xml::xmlKeyAmpHold()), 0.0, AmpMaxHoldSeconds);
}

double KickSynthDevice::ampDecaySeconds() const
{
    return ParameterMapper::mapExponential(value(Xml::xmlKeyAmpDecay()), AmpMinDecaySeconds, AmpMaxDecaySeconds);
}

double KickSynthDevice::ampReleaseSeconds() const
{
    // Zero is no release stage at all, as DrumAmpEnvelope reads it: the drum rings out
    const auto release = value(Xml::xmlKeyAmpRelease());
    return release > 0.0f ? ParameterMapper::mapExponential(release, AmpMinDecaySeconds, AmpMaxDecaySeconds) : 0.0;
}

bool KickSynthDevice::chromatic() const
{
    return m_chromatic;
}

void KickSynthDevice::setChromatic(bool chromatic)
{
    setContinuousParameterValue(Xml::xmlKeyChromaticMode().toStdString(), chromatic ? 1.0f : 0.0f);
}

std::vector<std::string> KickSynthDevice::modulationPriority() const
{
    return {
        Xml::xmlKeyDecay().toStdString(),
        Xml::xmlKeyDamping().toStdString(),
        Xml::xmlKeyTune().toStdString(),
        Xml::xmlKeyPitchDrop().toStdString(),
        Xml::xmlKeyTone().toStdString(),
        Xml::xmlKeyClick().toStdString(),
        Xml::xmlKeyFrontHead().toStdString(),
        Xml::xmlKeyMicPosition().toStdString(),
        Xml::xmlKeyDrive().toStdString(),
        Xml::xmlKeyLpfCutoff().toStdString(),
        Xml::xmlKeyHpfCutoff().toStdString()
    };
}

std::vector<std::string> KickSynthDevice::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyDecay().toStdString() };
}

} // namespace noteahead
