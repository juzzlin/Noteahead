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

#include "sampler.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../infra/audio/backend/audio_file_backend.hpp"
#include "../../infra/audio/backend/sndfile_backend.hpp"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

Sampler::Sampler(AudioFileBackendU audioFileBackend)
  : m_audioFileBackend { audioFileBackend ? std::move(audioFileBackend) : std::make_unique<SndFileBackend>() }
{
    m_voices.resize(MaxVoices);
    for (auto && sample : m_samples) {
        sample = nullptr;
    }
}

Sampler::~Sampler() = default;

std::string Sampler::name() const
{
    return Constants::samplerDeviceName().toStdString();
}

void Sampler::processMidiNoteOn(uint8_t note, uint8_t velocity)
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

void Sampler::processMidiNoteOff(uint8_t)
{
    // For now, samples just play till the end, no release phase
}

void Sampler::processMidiCc(uint8_t controller, uint8_t value)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (controller == 10) { // Panning
        m_globalPan = static_cast<float>(value) / 127.0f;
    }
}

void Sampler::processMidiAllNotesOff()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto && voice : m_voices) {
        voice.active = false;
    }
}

void Sampler::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    for (auto && voice : m_voices) {
        if (!voice.active || !voice.sample) {
            continue;
        }

        const auto & sampleData = voice.sample->data;
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

void Sampler::loadSample(uint8_t note, const std::string & filePath)
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

    AudioFileBackend::Info info {};
    if (!m_audioFileBackend->open(absolutePath.toStdString(), AudioFileBackend::Mode::Read, info)) {
        return;
    }

    auto sample = std::make_unique<Sample>();
    sample->filePath = filePath;
    sample->channels = info.channels;
    sample->sampleRate = info.samplerate;
    sample->data.resize(static_cast<size_t>(info.frames * info.channels));

    m_audioFileBackend->readFloat(std::span<float> { sample->data });
    m_audioFileBackend->close();

    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = std::move(sample);
}

void Sampler::clearSample(uint8_t note)
{
    if (note >= 128) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_samples.at(note) = nullptr;
}

const Sampler::Sample * Sampler::sample(uint8_t note) const
{
    if (note >= 128) {
        return nullptr;
    }
    return m_samples.at(note).get();
}

void Sampler::serializeToXml(QXmlStreamWriter & writer) const
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

void Sampler::deserializeFromXml(QXmlStreamReader & reader)
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
}

void Sampler::setProjectPath(const std::string & projectPath)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_projectPath = projectPath;
}

} // namespace noteahead
