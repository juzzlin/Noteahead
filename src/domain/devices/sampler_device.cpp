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

SamplerDevice::SamplerDevice(AudioFileReaderU audioFileReader)
  : m_audioFileReader { audioFileReader ? std::move(audioFileReader) : std::make_unique<SndFileReader>() }
{
    m_voices.resize(m_maxVoices);
    for (auto && sample : m_samples) {
        sample = nullptr;
    }
}

SamplerDevice::~SamplerDevice() = default;

std::string SamplerDevice::name() const
{
    return Constants::samplerDeviceName().toStdString();
}

void SamplerDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    if (note >= 128 || !m_samples.at(note)) {
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
            voice.position = 0.0;
            voice.velocity = static_cast<float>(velocity) / 127.0f;
            voice.pan = m_globalPan;
            voice.volume = m_globalVolume;
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            voice.lpL = voice.hpL = voice.bpL = 0.0f;
            voice.lpR = voice.hpR = voice.bpR = 0.0f;
            voice.hpfLpL = voice.hpfHpL = voice.hpfBpL = 0.0f;
            voice.hpfLpR = voice.hpfHpR = voice.hpfBpR = 0.0f;
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
        if (voice.active && voice.note == note && !voice.releasing) {
            voice.releasing = true;
        }
    }
}

void SamplerDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    if (m_channelMode) {
        // channel is 0-indexed (0-15)
        const size_t note = 36 + channel;
        if (note < 128 && m_samples.at(note)) {
            if (controller == 10) { // Panning
                m_samples.at(note)->pan = static_cast<float>(value) / 127.0f;
            } else if (controller == 7) { // Volume
                m_samples.at(note)->volume = static_cast<float>(value) / 127.0f;
            } else if (controller == 74) { // Cutoff (LPF)
                m_samples.at(note)->cutoff = static_cast<float>(value) / 127.0f;
            } else if (controller == 81) { // General Purpose 6 (HPF)
                m_samples.at(note)->hpfCutoff = static_cast<float>(value) / 127.0f;
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
                }
            }
        } else if (controller == 7) { // Volume
            m_globalVolume = static_cast<float>(value) / 127.0f;
            // Update all active voices' volume
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.volume = m_globalVolume;
                }
            }
        } else if (controller == 74) { // Cutoff (LPF)
            m_globalCutoff = static_cast<float>(value) / 127.0f;
            // Update all active voices' cutoff
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.cutoff = m_globalCutoff;
                }
            }
        } else if (controller == 81) { // General Purpose 6 (HPF)
            m_globalHpfCutoff = static_cast<float>(value) / 127.0f;
            // Update all active voices' hpfCutoff
            for (auto && voice : m_voices) {
                if (voice.active) {
                    voice.hpfCutoff = m_globalHpfCutoff;
                }
            }
        }
    }
}

void SamplerDevice::processMidiAllNotesOff()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.releasing = true;
        }
    }
}

void SamplerDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    const float fadeStep = 1.0f / 256.0f; // ~5ms fade at 48k

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

            // Low-pass filter (Chamberlin SVF)
            const float combinedCutoff = std::clamp(voice.sample->cutoff + (voice.cutoff - 1.0f), 0.0f, 1.0f);
            if (combinedCutoff < 0.999f) {
                const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, combinedCutoff);
                const float f = 2.0f * std::sin(static_cast<float>(M_PI) * freq / static_cast<float>(sampleRate));
                const float q = 0.5f;

                // Left
                voice.hpL = left - voice.lpL - q * voice.bpL;
                voice.bpL += f * voice.hpL;
                voice.lpL += f * voice.bpL;
                left = voice.lpL;

                // Right
                voice.hpR = right - voice.lpR - q * voice.bpR;
                voice.bpR += f * voice.hpR;
                voice.lpR += f * voice.bpR;
                right = voice.lpR;
            }

            // High-pass filter (Chamberlin SVF)
            const float combinedHpfCutoff = std::clamp(voice.sample->hpfCutoff + voice.hpfCutoff, 0.0f, 1.0f);
            if (combinedHpfCutoff > 0.001f) {
                const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, combinedHpfCutoff);
                const float f = 2.0f * std::sin(static_cast<float>(M_PI) * freq / static_cast<float>(sampleRate));
                const float q = 0.5f;

                // Left
                voice.hpfHpL = left - voice.hpfLpL - q * voice.hpfBpL;
                voice.hpfBpL += f * voice.hpfHpL;
                voice.hpfLpL += f * voice.hpfBpL;
                left = voice.hpfHpL;

                // Right
                voice.hpfHpR = right - voice.hpfLpR - q * voice.hpfBpR;
                voice.hpfBpR += f * voice.hpfHpR;
                voice.hpfLpR += f * voice.hpfBpR;
                right = voice.hpfHpR;
            }

            // Apply de-clicking fade if releasing
            if (voice.releasing) {
                left *= voice.releaseGain;
                right *= voice.releaseGain;
                voice.releaseGain -= fadeStep;
                if (voice.releaseGain <= 0.0f) {
                    voice.active = false;
                    voice.releasing = false;
                    // Move to next voice since this one is done
                    break;
                }
            }

            // Apply velocity, voice base pan, and current MIDI CC pan
            const float sPan = (voice.sample->pan * 2.0f) - 1.0f;
            const float mPan = (voice.pan * 2.0f) - 1.0f;
            const float combinedPan = (std::clamp(sPan + mPan, -1.0f, 1.0f) + 1.0f) / 2.0f;
            
            // Combined volume: sample base volume * current MIDI CC volume * velocity
            const float combinedVolume = voice.sample->volume * voice.volume * voice.velocity;
            
            const float gainL = std::min(1.0f, 2.0f - combinedPan * 2.0f) * combinedVolume;
            const float gainR = std::min(1.0f, combinedPan * 2.0f) * combinedVolume;

            output[i * 2] += left * gainL;
            output[i * 2 + 1] += right * gainR;

            voice.position += pitchScale;
        }
    }
}

void SamplerDevice::loadSample(uint8_t note, const std::string & filePath)
{
    if (note >= 128) {
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
    sample->cutoff = 1.0f;
    sample->hpfCutoff = 0.0f;

    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = std::move(sample);
    emit dataChanged();
}

void SamplerDevice::clearSample(uint8_t note)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = nullptr;
    emit dataChanged();
}

const SamplerDevice::Sample * SamplerDevice::sample(uint8_t note) const
{
    if (note >= 128) {
        return nullptr;
    }
    return m_samples.at(note).get();
}

std::string SamplerDevice::absoluteFilePath(uint8_t note) const
{
    if (note >= 128 || !m_samples.at(note)) {
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
    if (note >= 128 || !m_samples.at(note)) {
        return 0.5f;
    }
    return m_samples.at(note)->pan;
}

void SamplerDevice::setSamplePan(uint8_t note, float pan)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_samples.at(note)) {
        m_samples.at(note)->pan = std::clamp(pan, 0.0f, 1.0f);
        emit dataChanged();
    }
}

float SamplerDevice::sampleVolume(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (note >= 128 || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->volume;
}

void SamplerDevice::setSampleVolume(uint8_t note, float volume)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_samples.at(note)) {
        m_samples.at(note)->volume = std::clamp(volume, 0.0f, 1.0f);
        emit dataChanged();
    }
}

float SamplerDevice::sampleCutoff(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (note >= 128 || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->cutoff;
}

void SamplerDevice::setSampleCutoff(uint8_t note, float cutoff)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_samples.at(note)) {
        m_samples.at(note)->cutoff = std::clamp(cutoff, 0.0f, 1.0f);
        emit dataChanged();
    }
}

float SamplerDevice::sampleHpfCutoff(uint8_t note) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (note >= 128 || !m_samples.at(note)) {
        return 0.0f;
    }
    return m_samples.at(note)->hpfCutoff;
}

void SamplerDevice::setSampleHpfCutoff(uint8_t note, float cutoff)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_samples.at(note)) {
        m_samples.at(note)->hpfCutoff = std::clamp(cutoff, 0.0f, 1.0f);
        emit dataChanged();
    }
}

bool SamplerDevice::channelMode() const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_channelMode;
}

void SamplerDevice::setChannelMode(bool enabled)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_channelMode != enabled) {
        m_channelMode = enabled;
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
    writer.writeStartElement(Constants::NahdXml::xmlKeySampler());
    writer.writeAttribute(Constants::NahdXml::xmlKeyId(), QString::number(id()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChannelMode(), m_channelMode ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    for (uint8_t note = 0; note < 128; note++) {
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
            writer.writeAttribute(Constants::NahdXml::xmlKeyPan(), QString::number(std::round((s->pan * 200.0f) - 100.0f)));
            writer.writeAttribute(Constants::NahdXml::xmlKeyVolume(), QString::number(std::round(s->volume * 100.0f)));
            writer.writeAttribute(Constants::NahdXml::xmlKeyCutoff(), QString::number(std::round(s->cutoff * 100.0f)));
            writer.writeAttribute(Constants::NahdXml::xmlKeyHpfCutoff(), QString::number(std::round(s->hpfCutoff * 100.0f)));
            writer.writeEndElement();
        }
    }

    writer.writeEndElement(); // Sampler
}

void SamplerDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    const auto idAttr = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyId(), false);
    if (idAttr.has_value()) {
        setId(idAttr.value());
    }

    const auto channelModeAttr = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyChannelMode(), false);
    if (channelModeAttr.has_value()) {
        m_channelMode = channelModeAttr.value() == Constants::NahdXml::xmlValueTrue();
    }

    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == Constants::NahdXml::xmlKeySampler())) {
        if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeySample()) {
            const auto note = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
            const auto path = reader.attributes().value(Constants::NahdXml::xmlKeySamplePath()).toString();
            const auto pan = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyPan(), false);
            const auto volume = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyVolume(), false);
            const auto cutoff = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyCutoff(), false);
            const auto hpfCutoff = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyHpfCutoff(), false);
            if (note.has_value()) {
                loadSample(static_cast<uint8_t>(note.value()), path.toStdString());
                if (pan.has_value()) {
                    setSamplePan(static_cast<uint8_t>(note.value()), (static_cast<float>(pan.value()) + 100.0f) / 200.0f);
                }
                if (volume.has_value()) {
                    setSampleVolume(static_cast<uint8_t>(note.value()), static_cast<float>(volume.value()) / 100.0f);
                }
                if (cutoff.has_value()) {
                    setSampleCutoff(static_cast<uint8_t>(note.value()), static_cast<float>(cutoff.value()) / 100.0f);
                }
                if (hpfCutoff.has_value()) {
                    setSampleHpfCutoff(static_cast<uint8_t>(note.value()), static_cast<float>(hpfCutoff.value()) / 100.0f);
                }
            }
        }
        reader.readNext();
    }
    emit dataChanged();
}

void SamplerDevice::saveState()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (size_t i = 0; i < 128; ++i) {
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
    for (size_t i = 0; i < 128; ++i) {
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
