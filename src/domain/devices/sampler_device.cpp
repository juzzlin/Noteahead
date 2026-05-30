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

#include "sampler_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/audio/backend/sndfile_reader.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

SamplerDevice::Sample::Sample()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, -100, 100, 0, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVolume().toStdString(), 1.0f, 0, 100, 100, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCutoff().toStdString(), 1.0f, 0, 100, 100, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 100, 0, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStartOffset().toStdString(), 0.0f, 0, 60000, 0, 1 });
}

SamplerDevice::SamplerDevice(std::string name, AudioFileReaderU audioFileReader)
  : m_name { std::move(name) }
  , m_audioFileReader { audioFileReader ? std::move(audioFileReader) : std::make_unique<SndFileReader>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyChannelMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    m_voices.resize(m_maxVoices);
    for (auto && sample : m_samples) {
        sample = nullptr;
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());
    SamplerDevice::syncParameters();
}

SamplerDevice::~SamplerDevice() = default;

SamplerDevice::Voice::Voice()
{
    lpf = std::make_shared<LowPassFilterEffect>();
    hpf = std::make_shared<HighPassFilterEffect>();
    volumeEffect = std::make_shared<VolumeEffect>();
    panningEffect = std::make_shared<PanningEffect>();
    effects = { lpf, hpf, volumeEffect, panningEffect };
}

void SamplerDevice::updateVoiceEffects(Voice & voice)
{
    const float sPan = (voice.sample->pan * 2.0f) - 1.0f;
    const float mPan = (voice.pan * 2.0f) - 1.0f;
    const float combinedPan = (std::clamp(sPan + mPan, -1.0f, 1.0f) + 1.0f) / 2.0f;
    voice.panningEffect->setPan(combinedPan);

    const float combinedVolume = voice.sample->volume * voice.velocity;
    voice.volumeEffect->setVolume(combinedVolume);

    voice.lpf->setCutoff(std::clamp(voice.sample->cutoff + (voice.cutoff - 1.0f), 0.0f, 1.0f));
    voice.hpf->setCutoff(std::clamp(voice.sample->hpfCutoff + voice.hpfCutoff, 0.0f, 1.0f));
}

std::string SamplerDevice::name() const
{
    return m_name;
}

std::string SamplerDevice::category() const
{
    return Constants::NahdXml::xmlValueSamplers().toStdString();
}

std::string SamplerDevice::typeName() const
{
    return Constants::samplerDeviceName().toStdString();
}

std::string SamplerDevice::typeId() const
{
    return typeIdString();
}

void SamplerDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };

    if (note >= maxSamples || !m_samples.at(note)) {
        return;
    }

    // Stop any existing voices playing the same note (monophonic per note for now)
    // For de-clicking, we immediately stop it and let the new one start
    // A better way would be to let it finish its fade, but we need to find a free voice instead.
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.active = false;
        }
    }

    // Find an inactive voice
    for (auto && voice : m_voices) {
        if (!voice.active) {
            voice.note = note;
            voice.sample = m_samples.at(note).get();
            voice.position = voice.sample->startOffset * voice.sample->sampleRate;
            voice.velocity = static_cast<float>(velocity) / 127.0f;
            voice.pan = panInternal();
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            for (auto && effect : voice.effects) {
                effect->reset();
            }

            updateVoiceEffects(voice);

            voice.releasing = false;
            voice.releaseGain = 1.0f;
            voice.active = true;
            return;
        }
    }
}

void SamplerDevice::processMidiNoteOff(uint8_t note)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.releasing = true;
        }
    }
}

void SamplerDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            m_globalCutoff = m_manualGlobalCutoff;
            m_globalHpfCutoff = m_manualGlobalHpfCutoff;

            for (auto && sample : m_samples) {
                if (sample) {
                    sample->pan = sample->manualPan;
                    sample->volume = sample->manualVolume;
                    sample->cutoff = sample->manualCutoff;
                    sample->hpfCutoff = sample->manualHpfCutoff;

                    if (auto p = sample->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p)
                        p->get().setValue(sample->pan);
                    if (auto p = sample->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p)
                        p->get().setValue(sample->volume);
                    if (auto p = sample->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
                        p->get().setValue(sample->cutoff);
                    if (auto p = sample->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
                        p->get().setValue(sample->hpfCutoff);
                }
            }

            for (auto && voice : m_voices) {
                if (voice.active && voice.sample) {
                    voice.pan = panInternal();
                    voice.cutoff = m_globalCutoff;
                    voice.hpfCutoff = m_globalHpfCutoff;
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        } else if (m_channelMode) {
            // channel is 0-indexed (0-15)
            const size_t note = 36 + channel;
            if (note < maxSamples && m_samples.at(note)) {
                const float val = static_cast<float>(value) / 127.0f;
                if (controller == static_cast<uint8_t>(Controller::PanMSB)) { // Panning
                    m_samples.at(note)->pan = val;
                    if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p)
                        p->get().setValue(val);
                } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) { // Volume
                    m_samples.at(note)->volume = val;
                    if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p)
                        p->get().setValue(val);
                } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff (LPF)
                    m_samples.at(note)->cutoff = val;
                    if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
                        p->get().setValue(val);
                } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // General Purpose 6 (HPF)
                    m_samples.at(note)->hpfCutoff = val;
                    if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
                        p->get().setValue(val);
                }
                // Update active voices for this specific note
                for (auto && voice : m_voices) {
                    if (voice.active && voice.note == note) {
                        if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                            voice.pan = m_samples.at(note)->pan;
                        } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) {
                            voice.cutoff = m_samples.at(note)->cutoff;
                        } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) {
                            voice.hpfCutoff = m_samples.at(note)->hpfCutoff;
                        }
                        updateVoiceEffects(voice);
                    }
                }
            }
        } else {
            if (controller == static_cast<uint8_t>(Controller::PanMSB)) { // Panning
                changed |= updatePanParameter(static_cast<float>(value) / 127.0f, false);
                // Update all active voices' pan
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.pan = panInternal();
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) { // Volume
                changed |= updateVolumeParameter(static_cast<float>(value) / 127.0f, false);
                // Update all active voices' volume
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff (LPF)
                m_globalCutoff = static_cast<float>(value) / 127.0f;
                // Update all active voices' cutoff
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.cutoff = m_globalCutoff;
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // General Purpose 6 (HPF)
                m_globalHpfCutoff = static_cast<float>(value) / 127.0f;
                // Update all active voices' hpfCutoff
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.hpfCutoff = m_globalHpfCutoff;
                        updateVoiceEffects(voice);
                    }
                }
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void SamplerDevice::processMidiAllNotesOff()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        for (auto && voice : m_voices) {
            voice.active = false;
        }

        updatePanParameter(manualPanInternal(), false);
        updateVolumeParameter(manualVolumeInternal(), false);
        updateGainParameter(manualGainInternal(), false);
        m_globalCutoff = m_manualGlobalCutoff;
        m_globalHpfCutoff = m_manualGlobalHpfCutoff;

        for (auto && sample : m_samples) {
            if (sample) {
                sample->pan = sample->manualPan;
                sample->volume = sample->manualVolume;
                sample->cutoff = sample->manualCutoff;
                sample->hpfCutoff = sample->manualHpfCutoff;

                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p)
                    p->get().setValue(sample->pan);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p)
                    p->get().setValue(sample->volume);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
                    p->get().setValue(sample->cutoff);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
                    p->get().setValue(sample->hpfCutoff);
            }
        }

        // Update active voices to reflect the reset values
        for (auto && voice : m_voices) {
            if (voice.active && voice.sample) {
                voice.pan = panInternal();
                voice.cutoff = m_globalCutoff;
                voice.hpfCutoff = m_globalHpfCutoff;
                updateVoiceEffects(voice);
            }
        }
    }

    emit dataChanged();
}

void SamplerDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    std::vector<double> buffer(context.frameCount * 2, 0.0);

    const float fadeStep = 1.0f / 256.0f;

    for (auto && voice : m_voices) {
        if (!voice.active || !voice.sample || !voice.sample->data) {
            continue;
        }

        for (auto && effect : voice.effects) {
            effect->setSampleRate(context.sampleRate);
        }

        const auto & sampleData { *voice.sample->data };
        const int channels = voice.sample->channels;
        const float pitchScale = static_cast<float>(voice.sample->sampleRate) / static_cast<float>(context.sampleRate);

        for (uint32_t i = 0; i < context.frameCount; i++) {
            const double currentPos = voice.position;
            const size_t index = static_cast<size_t>(currentPos);
            const float fract = static_cast<float>(currentPos - index);

            if ((index + 1) * static_cast<size_t>(channels) >= sampleData.size()) {
                voice.active = false;
                break;
            }

            double left = 0.0;
            double right = 0.0;

            if (channels == 1) {
                const double s0 = static_cast<double>(sampleData.at(index));
                const double s1 = static_cast<double>(sampleData.at(index + 1));
                left = right = s0 + (s1 - s0) * fract;
            } else if (channels == 2) {
                const double l0 = static_cast<double>(sampleData.at(index * 2));
                const double l1 = static_cast<double>(sampleData.at((index + 1) * 2));
                const double r0 = static_cast<double>(sampleData.at(index * 2 + 1));
                const double r1 = static_cast<double>(sampleData.at((index + 1) * 2 + 1));
                left = l0 + (l1 - l0) * fract;
                right = r0 + (r1 - r0) * fract;
            }

            for (auto && effect : voice.effects) {
                effect->process(left, right);
            }

            if (voice.releasing) {
                left *= static_cast<double>(voice.releaseGain);
                right *= static_cast<double>(voice.releaseGain);
                voice.releaseGain -= fadeStep;
                if (voice.releaseGain <= 0.0f) {
                    voice.active = false;
                    voice.releasing = false;
                    break;
                }
            }

            buffer[i * 2] += left * static_cast<double>(linearGainInternal());
            buffer[i * 2 + 1] += right * static_cast<double>(linearGainInternal());

            voice.position += static_cast<double>(pitchScale);
        }
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        context.buffer[i * 2] += buffer[i * 2] * volumeInternal();
        context.buffer[i * 2 + 1] += buffer[i * 2 + 1] * volumeInternal();
    }
}

bool SamplerDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & voice : m_voices) {
        if (voice.active && voice.sample && voice.sample->data) {
            return true;
        }
    }
    return false;
}

void SamplerDevice::reset()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        Device::reset();
        resetAudio();
        syncParameters();
    }

    emit dataChanged();
}

void SamplerDevice::resetAudio()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        for (auto && voice : m_voices) {
            voice.active = false;
        }
    }
}

void SamplerDevice::loadSample(uint8_t note, const std::string & filePath)
{
    if (note >= maxSamples) {
        return;
    }

    const auto absolutePath = [this, &filePath]() {
        const auto path = QString::fromStdString(filePath);
        if (QFileInfo { path }.isRelative() && !m_projectPath.empty()) {
            return QFileInfo { QDir { QString::fromStdString(m_projectPath) }, path }.absoluteFilePath();
        }
        return path;
    }();

    // We always reload the sample to support cases where the file on disk has changed (e.g. after recording)
    AudioFileReader::Info info {};
    if (!m_audioFileReader->open(absolutePath.toStdString(), AudioFileReader::Mode::Read, info)) {
        return;
    }

    const auto data = std::make_shared<std::vector<float>>();
    data->resize(static_cast<size_t>(info.frames * info.channels));
    m_audioFileReader->readFloat(std::span<float> { *data });
    m_audioFileReader->close();

    auto sample = std::make_unique<Sample>();
    sample->filePath = filePath;
    sample->channels = info.channels;
    sample->sampleRate = info.samplerate;
    sample->data = std::move(data);

    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        m_samples.at(note) = std::move(sample);
    }

    emit dataChanged();
}

void SamplerDevice::clearSample(uint8_t note)
{
    if (note >= maxSamples) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        m_samples.at(note) = nullptr;
    }
    emit dataChanged();
}

const SamplerDevice::Sample * SamplerDevice::sample(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples) {
        return nullptr;
    }
    return m_samples.at(note).get();
}

std::string SamplerDevice::absoluteFilePath(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return "";
    }

    const auto filePath = m_samples.at(note)->filePath;
    const auto path = QString::fromStdString(filePath);
    if (QFileInfo { path }.isRelative() && !m_projectPath.empty()) {
        return QFileInfo { QDir { QString::fromStdString(m_projectPath) }, path }.absoluteFilePath().toStdString();
    }
    return filePath;
}

float SamplerDevice::samplePan(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.5f;
    }
    return m_samples.at(note)->pan;
}

void SamplerDevice::setSamplePan(uint8_t note, float pan)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
                p->get().setValue(pan);
                m_samples.at(note)->pan = p->get().value();
                m_samples.at(note)->manualPan = m_samples.at(note)->pan;
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleVolume(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->volume;
}

void SamplerDevice::setSampleVolume(uint8_t note, float volume)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) {
                p->get().setValue(volume);
                m_samples.at(note)->volume = p->get().value();
                m_samples.at(note)->manualVolume = m_samples.at(note)->volume;
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleCutoff(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->cutoff;
}

void SamplerDevice::setSampleCutoff(uint8_t note, float cutoff)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) {
                p->get().setValue(cutoff);
                m_samples.at(note)->cutoff = p->get().value();
                m_samples.at(note)->manualCutoff = m_samples.at(note)->cutoff;
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleHpfCutoff(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0f;
    }
    return m_samples.at(note)->hpfCutoff;
}

void SamplerDevice::setSampleHpfCutoff(uint8_t note, float cutoff)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
                p->get().setValue(cutoff);
                m_samples.at(note)->hpfCutoff = p->get().value();
                m_samples.at(note)->manualHpfCutoff = m_samples.at(note)->hpfCutoff;
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

double SamplerDevice::sampleStartOffset(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0;
    }
    return m_samples.at(note)->startOffset;
}

void SamplerDevice::setSampleStartOffset(uint8_t note, double offset)
{
    if (note >= maxSamples) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyStartOffset().toStdString()); p) {
                p->get().setValue(static_cast<float>(offset / 60.0));
                m_samples.at(note)->startOffset = static_cast<double>(p->get().value()) * 60.0;
            }
        }
    }
    emit dataChanged();
}

double SamplerDevice::sampleDuration(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note) || !m_samples.at(note)->data) {
        return 0.0;
    }
    const auto & s = m_samples.at(note);
    return static_cast<double>(s->data->size() / static_cast<size_t>(s->channels)) / static_cast<double>(s->sampleRate);
}

bool SamplerDevice::channelMode() const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_channelMode;
}

void SamplerDevice::setChannelMode(bool enabled)
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
            p->get().setValue(enabled ? 1.0f : 0.0f);
            m_channelMode = p->get().value() > 0.5f;
        }
    }
    emit dataChanged();
}

double SamplerDevice::playbackPosition(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto const & voice : m_voices) {
        if (voice.active && voice.note == note && voice.sample && voice.sample->data) {
            const size_t totalFrames = voice.sample->data->size() / static_cast<size_t>(voice.sample->channels);
            if (totalFrames > 0) {
                return voice.position / static_cast<double>(totalFrames);
            }
        }
    }
    return 0.0;
}

bool SamplerDevice::isFinished(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto const & voice : m_voices) {
        if (voice.active && voice.note == note) {
            return false;
        }
    }
    return true;
}

void SamplerDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeySamples());
    for (uint8_t note = 0; note < maxSamples; note++) {
        if (const auto & s = m_samples.at(note)) {
            writer.writeStartElement(Constants::NahdXml::xmlKeySample());
            writer.writeAttribute(Constants::NahdXml::xmlKeyNote(), QString::number(note));

            const auto path = [this, &s]() {
                const auto p = QString::fromStdString(s->filePath);
                if (!m_projectPath.empty() && QFileInfo { p }.isAbsolute()) {
                    return QDir { QString::fromStdString(m_projectPath) }.relativeFilePath(p);
                }
                return p;
            }();

            writer.writeAttribute(Constants::NahdXml::xmlKeySamplePath(), path);

            s->serializeParametersToXml(writer);

            writer.writeEndElement();
        }
    }
    writer.writeEndElement(); // Samples
    writer.writeEndElement(); // Device
}

void SamplerDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    deserializeAttributesFromXml(reader);

    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyParameters()) {
            deserializeParametersFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyInsertEffects()) {
            insertEffectRack().deserializeEffectsFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyParameter()) {
            deserializeParameter(reader);
        } else if (name == Constants::NahdXml::xmlKeySamples()) {
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeySample()) {
                    const auto note = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
                    const auto path = reader.attributes().value(Constants::NahdXml::xmlKeySamplePath()).toString();
                    if (note.has_value()) {
                        loadSample(static_cast<uint8_t>(note.value()), path.toStdString());
                        std::lock_guard<std::recursive_mutex> lock { mutex() };
                        if (const auto s = m_samples.at(note.value()).get(); s) {
                            s->deserializeParametersFromXml(reader);
                            // Sync internal fields from parameters
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p)
                                s->pan = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p)
                                s->volume = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
                                s->cutoff = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
                                s->hpfCutoff = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyStartOffset().toStdString()); p)
                                s->startOffset = static_cast<double>(p->get().value()) * 60.0;

                            s->manualPan = s->pan;
                            s->manualVolume = s->volume;
                            s->manualCutoff = s->cutoff;
                            s->manualHpfCutoff = s->hpfCutoff;
                        }
                    }
                    if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeySample()) {
                        reader.skipCurrentElement();
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        } else {
            reader.skipCurrentElement();
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        // Sync global fields
        syncParameters();

        // Update manual fallback values for MIDI CC reset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualGlobalCutoff = m_globalCutoff;
        m_manualGlobalHpfCutoff = m_globalHpfCutoff;
    }

    emit dataChanged();
}

void SamplerDevice::saveState()
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (size_t i = 0; i < maxSamples; i++) {
        if (m_samples.at(i)) {
            m_savedSamples.at(i) = std::make_unique<Sample>(*m_samples.at(i));
        } else {
            m_savedSamples.at(i) = nullptr;
        }
    }
}

void SamplerDevice::restoreState()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        for (size_t i = 0; i < maxSamples; i++) {
            m_samples.at(i) = std::move(m_savedSamples.at(i));
            m_savedSamples.at(i) = nullptr;
        }
    }
    emit dataChanged();
}

void SamplerDevice::setProjectPath(const std::string & projectPath)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_projectPath = projectPath;
}

void SamplerDevice::setPan(float pan)
{
    Device::setPan(pan);
}

void SamplerDevice::setVolume(float volume)
{
    Device::setVolume(volume);
}

float SamplerDevice::gain() const
{
    return Device::gain();
}

void SamplerDevice::setGain(float gain)
{
    Device::setGain(gain);
}

void SamplerDevice::syncParameters()
{
    Device::syncParameters();
    if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
        m_channelMode = p->get().value() > 0.5f;
    }

    // Update active voices with new global parameters
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.pan = panInternal();
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            updateVoiceEffects(voice);
        }
    }
}

} // namespace noteahead
