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

#include "string_ensemble_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Voicing of one register: which octave tap it draws from and the fixed filter that names it.
//!
//! These are the numbers to reach for when re-voicing the instrument by ear; nothing else in the
//! signal path encodes the character of a register.
struct RegisterVoicing
{
    int octaveOffset; //!< -1 = 16', 0 = 8', 1 = 4'
    bool bassSection;
    double frequency;
    double q;
    double gainDb; //!< Only meaningful for the shaping filters
};

enum class VoicingFilter
{
    HighCut,
    Bell,
    HighShelf
};

struct RegisterDefinition
{
    RegisterVoicing voicing;
    VoicingFilter filter;
};

// Contrabass and Cello are the bass section; Horn, Viola, Trumpet and Violin the upper one.
constexpr std::array<RegisterDefinition, 6> registerDefinitions {
    RegisterDefinition { { -1, true, 300.0, 0.707, 0.0 }, VoicingFilter::HighCut }, // Contrabass 16'
    RegisterDefinition { { 0, true, 800.0, 0.707, 0.0 }, VoicingFilter::HighCut }, // Cello 8'
    RegisterDefinition { { -1, false, 700.0, 0.707, 0.0 }, VoicingFilter::HighCut }, // Horn 16'
    RegisterDefinition { { 0, false, 1200.0, 0.9, 4.0 }, VoicingFilter::Bell }, // Viola 8'
    RegisterDefinition { { 0, false, 1400.0, 2.0, 9.0 }, VoicingFilter::Bell }, // Trumpet 8'
    RegisterDefinition { { 1, false, 2500.0, 0.707, 6.0 }, VoicingFilter::HighShelf } // Violin 4'
};

constexpr double MinAttackSeconds { 0.005 };
constexpr double MaxAttackSeconds { 3.0 };
constexpr double MinReleaseSeconds { 0.02 };
constexpr double MaxReleaseSeconds { 6.0 };

//! Headroom for the summed registers before the ensemble.
constexpr double OutputScale { 0.25 };

} // namespace

StringEnsembleDevice::StringEnsembleDevice(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyContrabass().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCello().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyViola().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyViolin().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTrumpet().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHorn().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeyModulation().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPhaserEnabled().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVolumeBass().toStdString(), 0.7f, 0, 10000, 7000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.15f, 0, 10000, 1500, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.35f, 0, 10000, 3500, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPhaserColor().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPhaserRate().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    for (auto && key : m_keys) {
        key.gate.setSustainLevel(1.0);
        key.gate.setDecayTime(0.0);
    }

    m_activeKeys.reserve(KeyCount);

    StringEnsembleDevice::syncParameters();
}

StringEnsembleDevice::~StringEnsembleDevice() = default;

std::string StringEnsembleDevice::name() const
{
    return m_name;
}

std::string StringEnsembleDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string StringEnsembleDevice::typeName() const
{
    return Constants::stringEnsembleDeviceName().toStdString();
}

std::string StringEnsembleDevice::typeIdString()
{
    return "6b4c9e0a-2d17-4f83-9a55-c8e1b7d30f42";
}

std::string StringEnsembleDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> StringEnsembleDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" }
    };
}

void StringEnsembleDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    auto & key = m_keys.at(note);
    key.gate.setSampleRate(static_cast<double>(sampleRate()));
    key.gate.setAttackTime(ParameterMapper::mapExponential(m_crescendo, MinAttackSeconds, MaxAttackSeconds));
    key.gate.setReleaseTime(ParameterMapper::mapExponential(m_sustainLength, MinReleaseSeconds, MaxReleaseSeconds));
    key.velocity = static_cast<float>(velocity) / 127.0f;
    key.gate.trigger();

    if (!key.active) {
        key.active = true;
        m_activeKeys.push_back(note);
    }
}

void StringEnsembleDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    releaseKey(note);
}

void StringEnsembleDevice::releaseKey(uint8_t note)
{
    auto & key = m_keys.at(note);
    if (key.active) {
        key.gate.setReleaseTime(ParameterMapper::mapExponential(m_sustainLength, MinReleaseSeconds, MaxReleaseSeconds));
        key.gate.release();
    }
}

void StringEnsembleDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void StringEnsembleDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto note : m_activeKeys) {
        releaseKey(note);
    }
}

bool StringEnsembleDevice::registerEnabled(Register reg) const
{
    switch (reg) {
    case Register::Contrabass:
        return m_contrabassEnabled;
    case Register::Cello:
        return m_celloEnabled;
    case Register::Horn:
        return m_hornEnabled;
    case Register::Viola:
        return m_violaEnabled;
    case Register::Trumpet:
        return m_trumpetEnabled;
    case Register::Violin:
        return m_violinEnabled;
    }
    return false;
}

void StringEnsembleDevice::updateRegisterFilters(double sampleRate)
{
    if (std::abs(m_lastRegisterFilterSampleRate - sampleRate) < 0.1) {
        return;
    }
    m_lastRegisterFilterSampleRate = sampleRate;

    for (size_t i = 0; i < registerDefinitions.size(); i++) {
        const auto & definition = registerDefinitions.at(i);
        const auto & voicing = definition.voicing;
        auto & filter = m_registerFilters.at(i);
        switch (definition.filter) {
        case VoicingFilter::HighCut:
            filter.calculateHighCut(voicing.frequency, sampleRate, voicing.q);
            break;
        case VoicingFilter::Bell:
            filter.calculateBell(voicing.frequency, sampleRate, voicing.q, voicing.gainDb);
            break;
        case VoicingFilter::HighShelf:
            filter.calculateHighShelf(voicing.frequency, sampleRate, voicing.q, voicing.gainDb);
            break;
        }
    }
}

void StringEnsembleDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const double sRate = static_cast<double>(context.sampleRate);

    m_generator.setSampleRate(sRate);
    updateRegisterFilters(sRate);

    m_ensemble.setSampleRate(sRate);
    m_phaser.setSampleRate(sRate);

    m_lpfL.setSampleRate(sRate);
    m_lpfR.setSampleRate(sRate);
    m_hpfL.setSampleRate(sRate);
    m_hpfR.setSampleRate(sRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));

    m_panner.setPan(static_cast<double>(panInternal()));

    // Which octave taps are worth generating at all: an unselected register costs nothing.
    bool needsBass16 = false;
    bool needsBass8 = false;
    bool needsUpper16 = false;
    bool needsUpper8 = false;
    bool needsUpper4 = false;
    int enabledRegisters = 0;
    for (size_t i = 0; i < registerDefinitions.size(); i++) {
        if (!registerEnabled(static_cast<Register>(i))) {
            continue;
        }
        enabledRegisters++;
        const auto & voicing = registerDefinitions.at(i).voicing;
        if (voicing.bassSection && voicing.octaveOffset < 0) {
            needsBass16 = true;
        } else if (voicing.bassSection) {
            needsBass8 = true;
        } else if (voicing.octaveOffset < 0) {
            needsUpper16 = true;
        } else if (voicing.octaveOffset > 0) {
            needsUpper4 = true;
        } else {
            needsUpper8 = true;
        }
    }

    // Both the number of held keys and the number of selected registers add energy, so compensate
    // for each: without this a six-register chord clips long before the master volume is touched.
    const double keyGain = 1.0 / std::sqrt(static_cast<double>(std::max<size_t>(1, m_activeKeys.size())));
    const double registerGain = 1.0 / std::sqrt(static_cast<double>(std::max(1, enabledRegisters)));
    const double mixGain = keyGain * registerGain * OutputScale * linearGainInternal();

    for (uint32_t frame = 0; frame < context.frameCount; frame++) {
        m_generator.tick();

        double bass16 = 0.0;
        double bass8 = 0.0;
        double upper16 = 0.0;
        double upper8 = 0.0;
        double upper4 = 0.0;

        for (const auto note : m_activeKeys) {
            auto & key = m_keys.at(note);
            key.gate.setSampleRate(sRate);
            const double envelope = key.gate.nextSample();
            if (key.gate.isSilent()) {
                key.active = false;
                continue;
            }

            const double level = envelope * (1.0 - m_velocitySensitivity + m_velocitySensitivity * key.velocity);

            if (note < SplitNote) {
                if (needsBass16) {
                    bass16 += m_generator.saw(note, -1) * level;
                }
                if (needsBass8) {
                    bass8 += m_generator.saw(note, 0) * level;
                }
            } else {
                if (needsUpper16) {
                    upper16 += m_generator.saw(note, -1) * level;
                }
                if (needsUpper8) {
                    upper8 += m_generator.saw(note, 0) * level;
                }
                if (needsUpper4) {
                    upper4 += m_generator.saw(note, 1) * level;
                }
            }
        }

        double bassSum = 0.0;
        double upperSum = 0.0;
        if (m_contrabassEnabled) {
            bassSum += m_registerFilters.at(static_cast<size_t>(Register::Contrabass)).process(bass16);
        }
        if (m_celloEnabled) {
            bassSum += m_registerFilters.at(static_cast<size_t>(Register::Cello)).process(bass8);
        }
        if (m_hornEnabled) {
            upperSum += m_registerFilters.at(static_cast<size_t>(Register::Horn)).process(upper16);
        }
        if (m_violaEnabled) {
            upperSum += m_registerFilters.at(static_cast<size_t>(Register::Viola)).process(upper8);
        }
        if (m_trumpetEnabled) {
            upperSum += m_registerFilters.at(static_cast<size_t>(Register::Trumpet)).process(upper8);
        }
        if (m_violinEnabled) {
            upperSum += m_registerFilters.at(static_cast<size_t>(Register::Violin)).process(upper4);
        }

        const double mono = (bassSum * static_cast<double>(m_volumeBass) + upperSum) * mixGain;

        double left = mono;
        double right = mono;

        m_ensemble.process(left, right);
        m_phaser.process(left, right);

        m_panner.process(left, right);

        left = m_hpfL.process(m_lpfL.process(left));
        right = m_hpfR.process(m_lpfR.process(right));

        context.buffer[frame * 2] += left;
        context.buffer[frame * 2 + 1] += right;
    }

    std::erase_if(m_activeKeys, [this](uint8_t note) { return !m_keys.at(note).active; });

    if (!m_activeKeys.empty()) {
        m_tailFrames = context.sampleRate;
    } else if (m_tailFrames > context.frameCount) {
        m_tailFrames -= context.frameCount;
    } else {
        m_tailFrames = 0;
    }
}

bool StringEnsembleDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return !m_activeKeys.empty() || m_tailFrames > 0;
}

void StringEnsembleDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void StringEnsembleDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && key : m_keys) {
        key.gate.reset();
        key.active = false;
    }
    m_activeKeys.clear();
    m_tailFrames = 0;

    m_generator.reset();
    for (auto && filter : m_registerFilters) {
        filter.reset();
    }
    m_ensemble.reset();
    m_phaser.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
}

void StringEnsembleDevice::serializeToXml(ProjectWriter & writer) const
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

void StringEnsembleDevice::deserializeFromXml(ProjectReader & reader)
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

void StringEnsembleDevice::updateKeyTimes()
{
    const double attack = ParameterMapper::mapExponential(m_crescendo, MinAttackSeconds, MaxAttackSeconds);
    const double release = ParameterMapper::mapExponential(m_sustainLength, MinReleaseSeconds, MaxReleaseSeconds);
    for (auto && key : m_keys) {
        key.gate.setAttackTime(attack);
        key.gate.setReleaseTime(release);
    }
}

void StringEnsembleDevice::syncParameters()
{
    Device::syncParameters();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyContrabass().toStdString()); p) {
        m_contrabassEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyCello().toStdString()); p) {
        m_celloEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyViola().toStdString()); p) {
        m_violaEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyViolin().toStdString()); p) {
        m_violinEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyTrumpet().toStdString()); p) {
        m_trumpetEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHorn().toStdString()); p) {
        m_hornEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyModulation().toStdString()); p) {
        m_modulationEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPhaserEnabled().toStdString()); p) {
        m_phaserEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVolumeBass().toStdString()); p) {
        m_volumeBass = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_crescendo = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p) {
        m_sustainLength = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPhaserColor().toStdString()); p) {
        m_phaserColor = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPhaserRate().toStdString()); p) {
        m_phaserRate = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString()); p) {
        m_velocitySensitivity = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }

    m_ensemble.setEnabled(m_modulationEnabled);
    m_phaser.setEnabled(m_phaserEnabled);
    m_phaser.setColor(static_cast<double>(m_phaserColor));
    m_phaser.setRate(static_cast<double>(m_phaserRate));

    updateKeyTimes();
}

bool StringEnsembleDevice::contrabassEnabled() const
{
    return m_contrabassEnabled;
}

void StringEnsembleDevice::setContrabassEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyContrabass().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::celloEnabled() const
{
    return m_celloEnabled;
}

void StringEnsembleDevice::setCelloEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyCello().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::violaEnabled() const
{
    return m_violaEnabled;
}

void StringEnsembleDevice::setViolaEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyViola().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::violinEnabled() const
{
    return m_violinEnabled;
}

void StringEnsembleDevice::setViolinEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyViolin().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::trumpetEnabled() const
{
    return m_trumpetEnabled;
}

void StringEnsembleDevice::setTrumpetEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyTrumpet().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::hornEnabled() const
{
    return m_hornEnabled;
}

void StringEnsembleDevice::setHornEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyHorn().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::modulationEnabled() const
{
    return m_modulationEnabled;
}

void StringEnsembleDevice::setModulationEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyModulation().toStdString(), enabled ? 1 : 0);
}

bool StringEnsembleDevice::phaserEnabled() const
{
    return m_phaserEnabled;
}

void StringEnsembleDevice::setPhaserEnabled(bool enabled)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyPhaserEnabled().toStdString(), enabled ? 1 : 0);
}

float StringEnsembleDevice::volumeBass() const
{
    return m_volumeBass;
}

void StringEnsembleDevice::setVolumeBass(float volumeBass)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVolumeBass().toStdString(), volumeBass);
}

float StringEnsembleDevice::crescendo() const
{
    return m_crescendo;
}

void StringEnsembleDevice::setCrescendo(float crescendo)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAttack().toStdString(), crescendo);
}

float StringEnsembleDevice::sustainLength() const
{
    return m_sustainLength;
}

void StringEnsembleDevice::setSustainLength(float sustainLength)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyReleaseTime().toStdString(), sustainLength);
}

float StringEnsembleDevice::phaserColor() const
{
    return m_phaserColor;
}

void StringEnsembleDevice::setPhaserColor(float color)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPhaserColor().toStdString(), color);
}

float StringEnsembleDevice::phaserRate() const
{
    return m_phaserRate;
}

void StringEnsembleDevice::setPhaserRate(float rate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPhaserRate().toStdString(), rate);
}

float StringEnsembleDevice::velocitySensitivity() const
{
    return m_velocitySensitivity;
}

void StringEnsembleDevice::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

float StringEnsembleDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void StringEnsembleDevice::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float StringEnsembleDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void StringEnsembleDevice::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), cutoff);
}

} // namespace noteahead
