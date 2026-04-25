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
    m_voices.resize(MaxVoices);
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

    // Find an inactive voice or the oldest one (not implemented, just first free)
    for (auto && voice : m_voices) {
        if (!voice.active) {
            voice.note = note;
            voice.sample = m_samples.at(note).get();
            voice.position = 0;
            voice.velocity = static_cast<float>(velocity) / 127.0f;
            voice.active = true;
            return;
        }
    }
}

void SamplerDevice::processMidiNoteOff(uint8_t)
{
    // For now, samples just play till the end, no release phase
}

void SamplerDevice::processMidiCc(uint8_t controller, uint8_t value)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (controller == 10) { // Panning
        m_globalPan = static_cast<float>(value) / 127.0f;
    }
}

void SamplerDevice::processMidiAllNotesOff()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto && voice : m_voices) {
        voice.active = false;
    }
}

void SamplerDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    for (auto && voice : m_voices) {
        if (!voice.active || !voice.sample || !voice.sample->data) {
            continue;
        }

        const auto & sampleData = *voice.sample->data;
        const int channels = voice.sample->channels;
        const float pitchScale = static_cast<float>(voice.sample->sampleRate) / static_cast<float>(sampleRate);

        // Very basic linear interpolation / resampling if needed
        for (uint32_t i = 0; i < nFrames; i++) {
            const float currentPos = static_cast<float>(voice.position) * pitchScale;
            const size_t index = static_cast<size_t>(currentPos);

            if (index * static_cast<size_t>(channels) >= sampleData.size()) {
                voice.active = false;
                break;
            }

            float left = 0.0f;
            float right = 0.0f;

            if (channels == 1) {
                left = right = sampleData.at(index);
            } else if (channels == 2) {
                left = sampleData.at(index * 2);
                right = sampleData.at(index * 2 + 1);
            }

            // Apply velocity, voice pan, and global pan
            const float pan = voice.sample->pan * m_globalPan * 2.0f; // Simplified pan
            const float gainL = std::min(1.0f, 2.0f - pan * 2.0f) * voice.velocity;
            const float gainR = std::min(1.0f, pan * 2.0f) * voice.velocity;

            output[i * 2] += left * gainL;
            output[i * 2 + 1] += right * gainR;

            voice.position++;
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

    // Check if we already have this sample loaded for another note
    std::shared_ptr<const std::vector<float>> sharedData;
    int channels = 0;
    int sampleRate = 0;

    {
        std::lock_guard<std::mutex> lock { m_mutex };
        for (const auto & s : m_samples) {
            if (s && s->filePath == filePath) {
                sharedData = s->data;
                channels = s->channels;
                sampleRate = s->sampleRate;
                break;
            }
        }
    }

    if (!sharedData) {
        AudioFileReader::Info info {};
        if (!m_audioFileReader->open(absolutePath.toStdString(), AudioFileReader::Mode::Read, info)) {
            return;
        }

        auto data = std::make_shared<std::vector<float>>();
        data->resize(static_cast<size_t>(info.frames * info.channels));
        m_audioFileReader->readFloat(std::span<float> { *data });
        m_audioFileReader->close();

        sharedData = std::move(data);
        channels = info.channels;
        sampleRate = info.samplerate;
    }

    auto sample = std::make_unique<Sample>();
    sample->filePath = filePath;
    sample->channels = channels;
    sample->sampleRate = sampleRate;
    sample->data = std::move(sharedData);

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

void SamplerDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeySampler());

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
            writer.writeEndElement();
        }
    }

    writer.writeEndElement(); // Sampler
}

void SamplerDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == Constants::NahdXml::xmlKeySampler())) {
        if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeySample()) {
            const auto note = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
            const auto path = reader.attributes().value(Constants::NahdXml::xmlKeySamplePath()).toString();
            if (note.has_value()) {
                loadSample(static_cast<uint8_t>(note.value()), path.toStdString());
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
