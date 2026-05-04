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
    addParameter(Parameter { Constants::NahdXml::xmlKeyChannelMode().toStdString(), 0.0f, 0, 1, 0, 1 });

    m_voices.resize(m_maxVoices);
    for (auto && sample : m_samples) {
        sample = nullptr;
    }
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

    const float combinedVolume = voice.sample->volume * voice.volume * voice.velocity;
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

std::string SamplerDevice::typeId() const
{
    return "9dda4ff6-471b-11f1-9324-c701bfaf8258";
}

void SamplerDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    std::lock_guard<std::mutex> lock { m_mutex };

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
            voice.pan = m_globalPan;
            voice.volume = m_globalVolume;
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
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.releasing = true;
        }
    }
}

void SamplerDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    if (controller == 121) { // Reset All Controllers
        m_globalPan = m_manualGlobalPan;
        m_globalVolume = m_manualGlobalVolume;
        m_globalCutoff = m_manualGlobalCutoff;
        m_globalHpfCutoff = m_manualGlobalHpfCutoff;

        for (auto && sample : m_samples) {
            if (sample) {
                sample->pan = sample->manualPan;
                sample->volume = sample->manualVolume;
                sample->cutoff = sample->manualCutoff;
                sample->hpfCutoff = sample->manualHpfCutoff;

                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) p->get().setValue(sample->pan);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) p->get().setValue(sample->volume);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(sample->cutoff);
                if (auto p = sample->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) p->get().setValue(sample->hpfCutoff);
            }
        }
        
        for (auto && voice : m_voices) {
            if (voice.active && voice.sample) {
                voice.pan = m_globalPan;
                voice.volume = m_globalVolume;
                voice.cutoff = m_globalCutoff;
                voice.hpfCutoff = m_globalHpfCutoff;
                updateVoiceEffects(voice);
            }
        }
        emit dataChanged();
        return;
    }

    if (m_channelMode) {
        // channel is 0-indexed (0-15)
        const size_t note = 36 + channel;
        if (note < maxSamples && m_samples.at(note)) {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == 10) { // Panning
                m_samples.at(note)->pan = val;
                if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) p->get().setValue(val);
            } else if (controller == 7) { // Volume
                m_samples.at(note)->volume = val;
                if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) p->get().setValue(val);
            } else if (controller == 74) { // Cutoff (LPF)
                m_samples.at(note)->cutoff = val;
                if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(val);
            } else if (controller == 81) { // General Purpose 6 (HPF)
                m_samples.at(note)->hpfCutoff = val;
                if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) p->get().setValue(val);
            }
            // Update active voices for this specific note
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    if (controller == 10) {
                        voice.pan = m_samples.at(note)->pan;
                    } else if (controller == 7) {
                        voice.volume = m_samples.at(note)->volume;
                    } else if (controller == 74) {
                        voice.cutoff = m_samples.at(note)->cutoff;
                    } else if (controller == 81) {
                        voice.hpfCutoff = m_samples.at(note)->hpfCutoff;
                    }
                    updateVoiceEffects(voice);
                }
            }
        }
    } else {
        if (controller == 10) { // Panning
            m_globalPan = static_cast<float>(value) / 127.0f;
            // Update all active voices' pan
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.pan = m_globalPan;
                    updateVoiceEffects(voice);
                }
            }
        } else if (controller == 7) { // Volume
            m_globalVolume = static_cast<float>(value) / 127.0f;
            // Update all active voices' volume
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.volume = m_globalVolume;
                    updateVoiceEffects(voice);
                }
            }
        } else if (controller == 74) { // Cutoff (LPF)
            m_globalCutoff = static_cast<float>(value) / 127.0f;
            // Update all active voices' cutoff
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.cutoff = m_globalCutoff;
                    updateVoiceEffects(voice);
                }
            }
        } else if (controller == 81) { // General Purpose 6 (HPF)
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

void SamplerDevice::processMidiAllNotesOff()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto && voice : m_voices) {
        voice.active = false;
    }

    m_globalPan = m_manualGlobalPan;
    m_globalVolume = m_manualGlobalVolume;
    m_globalCutoff = m_manualGlobalCutoff;
    m_globalHpfCutoff = m_manualGlobalHpfCutoff;

    for (auto && sample : m_samples) {
        if (sample) {
            sample->pan = sample->manualPan;
            sample->volume = sample->manualVolume;
            sample->cutoff = sample->manualCutoff;
            sample->hpfCutoff = sample->manualHpfCutoff;

            if (auto p = sample->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) p->get().setValue(sample->pan);
            if (auto p = sample->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) p->get().setValue(sample->volume);
            if (auto p = sample->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(sample->cutoff);
            if (auto p = sample->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) p->get().setValue(sample->hpfCutoff);
        }
    }

    // Update active voices to reflect the reset values
    for (auto && voice : m_voices) {
        if (voice.active && voice.sample) {
            voice.pan = m_globalPan;
            voice.volume = m_globalVolume;
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            updateVoiceEffects(voice);
        }
    }

    emit dataChanged();
}

void SamplerDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    const float fadeStep = 1.0f / 256.0f;

    for (auto && voice : m_voices) {
        if (!voice.active || !voice.sample || !voice.sample->data) {
            continue;
        }

        const auto & sampleData = *voice.sample->data;
        const int channels = voice.sample->channels;
        const float pitchScale = static_cast<float>(voice.sample->sampleRate) / static_cast<float>(sampleRate);

        for (uint32_t i = 0; i < nFrames; i++) {
            const double currentPos = voice.position;
            const size_t index = static_cast<size_t>(currentPos);
            const float fract = static_cast<float>(currentPos - index);

            if ((index + 1) * static_cast<size_t>(channels) >= sampleData.size()) {
                voice.active = false;
                break;
            }

            float left = 0.0f;
            float right = 0.0f;

            if (channels == 1) {
                const float s0 = sampleData.at(index);
                const float s1 = sampleData.at(index + 1);
                left = right = s0 + (s1 - s0) * fract;
            } else if (channels == 2) {
                const float l0 = sampleData.at(index * 2);
                const float l1 = sampleData.at((index + 1) * 2);
                const float r0 = sampleData.at(index * 2 + 1);
                const float r1 = sampleData.at((index + 1) * 2 + 1);
                left = l0 + (l1 - l0) * fract;
                right = r0 + (r1 - r0) * fract;
            }

            for (auto && effect : voice.effects) {
                effect->process(left, right, sampleRate);
            }

            if (voice.releasing) {
                left *= voice.releaseGain;
                right *= voice.releaseGain;
                voice.releaseGain -= fadeStep;
                if (voice.releaseGain <= 0.0f) {
                    voice.active = false;
                    voice.releasing = false;
                    break;
                }
            }

            output[i * 2] += left;
            output[i * 2 + 1] += right;

            voice.position += pitchScale;
        }
    }
}

void SamplerDevice::reset()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    ParameterContainer::reset();
    for (auto && sample : m_samples) {
        sample = nullptr;
    }
    for (auto && voice : m_voices) {
        voice.active = false;
    }

    if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
        m_channelMode = p->get().value() > 0.5f;
    }

    emit dataChanged();
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

    auto data = std::make_shared<std::vector<float>>();
    data->resize(static_cast<size_t>(info.frames * info.channels));
    m_audioFileReader->readFloat(std::span<float> { *data });
    m_audioFileReader->close();

    auto sample = std::make_unique<Sample>();
    sample->filePath = filePath;
    sample->channels = info.channels;
    sample->sampleRate = info.samplerate;
    sample->data = std::move(data);

    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = std::move(sample);
    emit dataChanged();
}

void SamplerDevice::clearSample(uint8_t note)
{
    if (note >= maxSamples) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = nullptr;
    emit dataChanged();
}

const SamplerDevice::Sample * SamplerDevice::sample(uint8_t note) const
{
    if (note >= maxSamples) {
        return nullptr;
    }
    return m_samples.at(note).get();
}

std::string SamplerDevice::absoluteFilePath(uint8_t note) const
{
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
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
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
        emit dataChanged();
    }
}

float SamplerDevice::sampleVolume(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
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
        emit dataChanged();
    }
}

float SamplerDevice::sampleCutoff(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
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
        emit dataChanged();
    }
}

float SamplerDevice::sampleHpfCutoff(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
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
        emit dataChanged();
    }
}

double SamplerDevice::sampleStartOffset(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_samples.at(note)) {
        if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyStartOffset().toStdString()); p) {
            p->get().setValue(static_cast<float>(offset / 60.0));
            m_samples.at(note)->startOffset = static_cast<double>(p->get().value()) * 60.0;
        }
        emit dataChanged();
    }
}

double SamplerDevice::sampleDuration(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (note >= maxSamples || !m_samples.at(note) || !m_samples.at(note)->data) {
        return 0.0;
    }
    const auto & s = m_samples.at(note);
    return static_cast<double>(s->data->size() / static_cast<size_t>(s->channels)) / static_cast<double>(s->sampleRate);
}

bool SamplerDevice::channelMode() const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_channelMode;
}

void SamplerDevice::setChannelMode(bool enabled)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
        p->get().setValue(enabled ? 1.0f : 0.0f);
        m_channelMode = p->get().value() > 0.5f;
        emit dataChanged();
    }
}

double SamplerDevice::playbackPosition(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & voice : m_voices) {
        if (voice.active && voice.note == note) {
            return false;
        }
    }
    return true;
}

void SamplerDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);

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
        if (name == Constants::NahdXml::xmlKeyParameter()) {
            const auto paramName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            const auto value = reader.attributes().value(Constants::NahdXml::xmlKeyValue()).toInt();
            if (auto p = parameter(paramName); p) {
                p->get().setFromXml(value);
            }
            reader.skipCurrentElement();
        } else if (name == Constants::NahdXml::xmlKeySamples()) {
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeySample()) {
                    const auto note = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
                    const auto path = reader.attributes().value(Constants::NahdXml::xmlKeySamplePath()).toString();
                    if (note.has_value()) {
                        loadSample(static_cast<uint8_t>(note.value()), path.toStdString());
                        if (const auto s = m_samples.at(note.value()).get(); s) {
                            s->deserializeParametersFromXml(reader);
                            // Sync internal fields from parameters
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) s->pan = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) s->volume = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) s->cutoff = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) s->hpfCutoff = p->get().value();
                            if (auto p = s->parameter(Constants::NahdXml::xmlKeyStartOffset().toStdString()); p) s->startOffset = static_cast<double>(p->get().value()) * 60.0;
                            
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
    
    // Sync global fields
    if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
        m_channelMode = p->get().value() > 0.5f;
    }

    emit dataChanged();
}

void SamplerDevice::saveState()
{
    std::lock_guard<std::mutex> lock { m_mutex };
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
    std::lock_guard<std::mutex> lock { m_mutex };
    for (size_t i = 0; i < maxSamples; i++) {
        m_samples.at(i) = std::move(m_savedSamples.at(i));
        m_savedSamples.at(i) = nullptr;
    }
    emit dataChanged();
}

void SamplerDevice::setProjectPath(const std::string & projectPath)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_projectPath = projectPath;
}

} // namespace noteahead
