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

#include "bass_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cmath>

namespace noteahead {

void BassSynthDevice::Voice::reset()
{
    active = false;
    lpf.reset();
    hpf.reset();
    filterEg.reset();
    ampEg.reset();
    glideFrequency = 0.0;
}

void BassSynthDevice::Voice::trigger(uint8_t n, double freq, float vel, bool hasAccent, bool phaseSync)
{
    note = n;
    frequency = freq;
    velocity = vel;
    accent = hasAccent;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    active = true;

    if (phaseSync) {
        vco.sync(0.0);
        sub.sync(0.0);
    }
    lpf.reset();
    hpf.reset();
    filterEg.reset();
    ampEg.reset();
    filterEg.trigger();
    ampEg.trigger();
}

void BassSynthDevice::Voice::release()
{
    filterEg.release();
    ampEg.release();
}

BassSynthDevice::BassSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    // Initialize Parameters
    addParameter(Parameter(Constants::NahdXml::xmlKeyWaveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeyPitch().toStdString(), 0.5f, -1200, 1200, 0));
    addParameter(Parameter(Constants::NahdXml::xmlKeySubLevel().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySubOctave().toStdString(), 1.0f, 1, 2, 1, 1, Parameter::Type::Discrete));

    addParameter(Parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyEnvMod().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDecay().toStdString(), 0.5f, 0, 10000, 5000, 100));

    addParameter(Parameter(Constants::NahdXml::xmlKeyAccent().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySlide().toStdString(), 0.0f, 0, 10000, 0, 100));

    addParameter(Parameter(Constants::NahdXml::xmlKeyDistDrive().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDistTone().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyDistLevel().toStdString(), 1.0f, 0, 10000, 10000, 100));

    m_voice.vco.setWaveform(PolyBlepOscillator::Waveform::Saw);
    m_voice.sub.setWaveform(PolyBlepOscillator::Waveform::Square);

    m_voice.hpf.setMode(CascadedSvf::Mode::HighPass);

    m_voice.filterEg.setSustainLevel(0.0);
    m_voice.ampEg.setSustainLevel(0.0);
    m_voice.ampEg.setAttackTime(0.005); // Snappy but no click
    m_voice.filterEg.setAttackTime(0.005);

    BassSynthDevice::syncParameters();
}

BassSynthDevice::~BassSynthDevice() = default;

std::string BassSynthDevice::name() const
{
    return m_name;
}

std::string BassSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string BassSynthDevice::typeName() const
{
    return Constants::bassSynthDeviceName().toStdString();
}

std::string BassSynthDevice::typeIdString()
{
    return "7d9c1e4b-2f3a-4b5c-8d6e-9f0a1b2c3d4e";
}

std::string BassSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> BassSynthDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        { static_cast<uint8_t>(Controller::ChannelVolumeMSB), "Volume" },
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF" }
    };
}

void BassSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void BassSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void BassSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            m_lpfCutoff = m_manualLpfCutoff;
            m_hpfCutoff = m_manualHpfCutoff;

            if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p)
                p->get().setValue(m_lpfCutoff);
            if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p)
                p->get().setValue(m_hpfCutoff);

            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else {
            const float val = static_cast<float>(value) / 127.0f;

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff
                m_lpfCutoff = val;
                if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // HPF Cutoff
                m_hpfCutoff = val;
                if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void BassSynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void BassSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    m_voice.reset();
}

void BassSynthDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const uint32_t oversampledRate = context.sampleRate * 2;
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    if (!m_voice.active)
        return;

    m_voice.vco.setSampleRate(oversampledRate);
    m_voice.sub.setSampleRate(oversampledRate);
    m_voice.lpf.setSampleRate(oversampledRate);
    m_voice.hpf.setSampleRate(oversampledRate);
    m_voice.filterEg.setSampleRate(oversampledRate);
    m_voice.ampEg.setSampleRate(oversampledRate);

    m_voice.lpf.setResonance(m_voice.accent ? std::clamp(m_lpfResonance + m_accent * 0.2f, 0.0f, 1.0f) : m_lpfResonance);
    m_voice.lpf.setDrive(m_distDrive);

    // Slide constant
    if (oversampledRate != m_lastOversampledRate) {
        m_slideCoeff = m_slide > 0 ? 1.0 - std::pow(0.001, 1.0 / (ParameterMapper::mapExponential(m_slide, 0.01, 2.0) * oversampledRate)) : 1.0;
        m_lastOversampledRate = oversampledRate;
    }

    const double pbOffset = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * (m_pitchBendRange / 12.0);
    const double combinedPitchRatio = m_vcoBasePitchRatio * std::exp2(pbOffset);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        float l[2] { 0.0f, 0.0f };
        float r[2] { 0.0f, 0.0f };

        for (int os = 0; os < 2; os++) {
            m_voice.glideFrequency += (m_voice.frequency - m_voice.glideFrequency) * m_slideCoeff;

            const double vcoFreq = m_voice.glideFrequency * combinedPitchRatio;
            m_voice.vco.setFrequency(vcoFreq);

            double subVal = 0.0;
            if (m_subLevel >= 0.001f) {
                m_voice.sub.setFrequency(vcoFreq * m_subBasePitchRatio);
                subVal = m_voice.sub.nextSample() * m_subLevel;
            }

            const double filterEnv = m_voice.filterEg.nextSample();
            const double ampEnv = m_voice.ampEg.nextSample();

            const double vcoVal = m_voice.vco.nextSample();

            const double mix = vcoVal + subVal;

            // Filter
            double cutoffMod = filterEnv * m_envMod;
            if (m_voice.accent) {
                cutoffMod += m_accent * filterEnv * 0.5; // Accent boosts filter env effect
            }

            m_voice.lpf.setCutoff(std::clamp(m_lpfCutoff + cutoffMod, 0.0, 1.0));
            m_voice.hpf.setCutoff(m_hpfCutoff);

            float filtered = m_voice.hpf.process(m_voice.lpf.process(static_cast<float>(mix)));

            // Distortion
            if (m_distDrive > 0.0f) {
                float drive = 1.0f + m_distDrive * 10.0f;
                // Asymmetric distortion for more character
                filtered = (std::tanh(filtered * drive) + 0.1f * std::tanh(filtered * drive * 2.0f)) * m_distLevel;
                // Simple tone control (LPF)
                float alpha = 0.1f + m_distTone * 0.8f;
                m_distLpState += alpha * (filtered - m_distLpState);
                filtered = m_distLpState;
            }

            float finalSample = filtered * static_cast<float>(ampEnv) * linearGainInternal() * m_voice.velocity;
            if (m_voice.accent) {
                finalSample *= (1.0f + m_accent);
            }

            l[os] = finalSample * (1.0f - panInternal()) * 2.0f;
            r[os] = finalSample * panInternal() * 2.0f;
        }

        context.buffer[i * 2] += m_oversamplerL.process(l[0], l[1]) * volumeInternal();
        context.buffer[i * 2 + 1] += m_oversamplerR.process(r[0], r[1]) * volumeInternal();
    }

    if (m_voice.ampEg.state() == AdsrEnvelope::State::Idle) {
        m_voice.active = false;
    }
}

bool BassSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voice.active;
}

void BassSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    Device::reset();
    resetAudio();
    syncParameters();
}

void BassSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    m_voice.reset();
    m_oversamplerL.reset();
    m_oversamplerR.reset();
    m_distLpState = 0.0f;
}

void BassSynthDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
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

void BassSynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == QXmlStreamReader::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == QXmlStreamReader::StartElement) {
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

        // Update manual fallback values for MIDI CC reset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualLpfCutoff = m_lpfCutoff;
        m_manualHpfCutoff = m_hpfCutoff;
    }
    emit dataChanged();
}

void BassSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    double freq = midiNoteToFreq(note);
    bool hasAccent = velocity > 100;
    float vel = static_cast<float>(velocity) / 127.0f;

    if (m_voice.active) {
        // Legato / Slide
        m_voice.frequency = freq;
        m_voice.note = note;
        m_voice.velocity = vel;
        m_voice.accent = hasAccent;
        // Always trigger envelopes to ensure the attack/decay cycle starts again.
        // For slides, we don't reset to zero to keep it smooth, but we still trigger the attack phase.
        // If slide is 0, we reset the filter envelope for a snappy start, but not the amplitude to avoid clicks.
        if (m_slide == 0.0f || hasAccent) {
            m_voice.filterEg.reset();
        }

        if (hasAccent) {
            // Real 303 accent: decay is shorter
            m_voice.filterEg.setDecayTime(ParameterMapper::mapExponential(m_decay, 0.1, 10.0) * 0.5);
        } else {
            m_voice.filterEg.setDecayTime(ParameterMapper::mapExponential(m_decay, 0.1, 10.0));
        }

        m_voice.filterEg.trigger();
        m_voice.ampEg.trigger();
    } else {
        m_distLpState = 0.0f;
        m_oversamplerL.reset();
        m_oversamplerR.reset();
        if (hasAccent) {
            m_voice.filterEg.setDecayTime(ParameterMapper::mapExponential(m_decay, 0.1, 10.0) * 0.5);
        } else {
            m_voice.filterEg.setDecayTime(ParameterMapper::mapExponential(m_decay, 0.1, 10.0));
        }
        m_voice.trigger(note, freq, vel, hasAccent, true);
    }
}

void BassSynthDevice::handleNoteOff(uint8_t note)
{
    if (m_voice.active && m_voice.note == note) {
        m_voice.release();
    }
}

double BassSynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

void BassSynthDevice::syncParameters()
{
    Device::syncParameters();
    if (auto p = parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p)
        m_waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyPitch().toStdString()); p)
        m_tuning = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySubLevel().toStdString()); p)
        m_subLevel = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySubOctave().toStdString()); p)
        m_subOctave = static_cast<int>(p->get().xmlValue());

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p)
        m_lpfCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p)
        m_lpfResonance = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p)
        m_hpfCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyEnvMod().toStdString()); p)
        m_envMod = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p)
        m_decay = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeyAccent().toStdString()); p)
        m_accent = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySlide().toStdString()); p)
        m_slide = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeyDistDrive().toStdString()); p)
        m_distDrive = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDistTone().toStdString()); p)
        m_distTone = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDistLevel().toStdString()); p)
        m_distLevel = p->get().value();

    const double tuningOffset = ParameterMapper::mapCubicCentered(m_tuning * 2.0 - 1.0, -1200, 1200);
    m_vcoBasePitchRatio = std::exp2(tuningOffset / 1200.0);
    m_subBasePitchRatio = 1.0 / std::exp2(static_cast<double>(m_subOctave));

    m_lastOversampledRate = 0; // Force update of slideCoeff in processAudio

    m_voice.vco.setWaveform(m_waveform);
    const float decayTime = ParameterMapper::mapExponential(m_decay, 0.1, 10.0);
    m_voice.filterEg.setDecayTime(m_voice.accent ? decayTime * 0.5 : decayTime);
    m_voice.filterEg.setReleaseTime(0.005);
    m_voice.ampEg.setDecayTime(decayTime);
    m_voice.ampEg.setReleaseTime(0.005);
}

PolyBlepOscillator::Waveform BassSynthDevice::waveform() const
{
    return m_waveform;
}

void BassSynthDevice::setWaveform(PolyBlepOscillator::Waveform wave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(wave));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::tuning() const
{
    return m_tuning;
}

void BassSynthDevice::setTuning(float tuning)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyPitch().toStdString()); p) {
            p->get().setValue(tuning);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::subLevel() const
{
    return m_subLevel;
}

void BassSynthDevice::setSubLevel(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySubLevel().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

int BassSynthDevice::subOctave() const
{
    return m_subOctave;
}

void BassSynthDevice::setSubOctave(int octave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySubOctave().toStdString()); p) {
            p->get().setFromXml(octave);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void BassSynthDevice::setLpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualLpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void BassSynthDevice::setLpfResonance(float resonance)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p) {
            p->get().setValue(resonance);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void BassSynthDevice::setHpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualHpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::envMod() const
{
    return m_envMod;
}

void BassSynthDevice::setEnvMod(float mod)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyEnvMod().toStdString()); p) {
            p->get().setValue(mod);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::decay() const
{
    return m_decay;
}

void BassSynthDevice::setDecay(float decay)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
            p->get().setValue(decay);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::accent() const
{
    return m_accent;
}

void BassSynthDevice::setAccent(float accent)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyAccent().toStdString()); p) {
            p->get().setValue(accent);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::slide() const
{
    return m_slide;
}

void BassSynthDevice::setSlide(float slide)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeySlide().toStdString()); p) {
            p->get().setValue(slide);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::distDrive() const
{
    return m_distDrive;
}

void BassSynthDevice::setDistDrive(float drive)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyDistDrive().toStdString()); p) {
            p->get().setValue(drive);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::distTone() const
{
    return m_distTone;
}

void BassSynthDevice::setDistTone(float tone)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyDistTone().toStdString()); p) {
            p->get().setValue(tone);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float BassSynthDevice::distLevel() const
{
    return m_distLevel;
}

void BassSynthDevice::setDistLevel(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyDistLevel().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

} // namespace noteahead
