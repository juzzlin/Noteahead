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

#include "audio_file_recorder.hpp"
#include "backend/sndfile_reader.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace noteahead {

AudioFileRecorder::AudioFileRecorder(std::unique_ptr<AudioFileReader> writer)
  : m_writer { std::move(writer) }
{
    if (!m_writer) {
        m_writer = std::make_unique<SndFileReader>();
    }
}

AudioFileRecorder::~AudioFileRecorder()
{
    stop();
}

void AudioFileRecorder::start(const std::string & fileName, uint32_t sampleRate, uint32_t channelCount, size_t bufferSize)
{
    stop();

    AudioFileReader::Info info;
    info.samplerate = static_cast<int>(sampleRate);
    info.channels = static_cast<int>(channelCount);
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    if (!m_writer->open(fileName, AudioFileReader::Mode::Write, info)) {
        throw std::runtime_error("Error opening file for writing: " + fileName);
    }

    if (bufferSize > 0) {
        m_ringBuffer.resize(bufferSize);
    }
    m_ringBuffer.clear();

    m_stopThread = false;
    m_diskWriteThread = std::thread(&AudioFileRecorder::diskWriteLoop, this);
}

void AudioFileRecorder::stop()
{
    m_stopThread = true;
    if (m_diskWriteThread.joinable()) {
        m_diskWriteThread.join();
    }

    if (m_writer && m_writer->isOpen()) {
        m_writer->close();
    }
}

bool AudioFileRecorder::push(const int32_t * data, size_t count)
{
    return m_ringBuffer.push(data, count);
}

void AudioFileRecorder::diskWriteLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopThread || m_ringBuffer.readAvailable() > 0) {
        const size_t available = m_ringBuffer.readAvailable();
        if (available > 0) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const size_t read = m_ringBuffer.pop(tempBuffer.data(), toRead);
            if (read > 0 && m_writer->isOpen()) {
                m_writer->writeInt({ tempBuffer.data(), read });
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace noteahead
