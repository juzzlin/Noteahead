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

#include "audio_file_streamer.hpp"
#include "backend/sndfile_reader.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace noteahead {

AudioFileStreamer::AudioFileStreamer(std::unique_ptr<AudioFileReader> reader)
  : m_reader { std::move(reader) }
{
    if (!m_reader) {
        m_reader = std::make_unique<SndFileReader>();
    }
}

AudioFileStreamer::~AudioFileStreamer()
{
    stop();
}

void AudioFileStreamer::start(const std::string & fileName, size_t bufferSize, double startPosition)
{
    stop();

    AudioFileReader::Info info;
    if (!m_reader->open(fileName, AudioFileReader::Mode::Read, info)) {
        throw std::runtime_error("Error opening file for reading: " + fileName);
    }

    if (bufferSize > 0) {
        m_ringBuffer.resize(bufferSize);
    }
    m_ringBuffer.clear();

    m_playedFrames = 0;
    m_playbackPosition = 0.0;
    m_isFinished = false;

    if (startPosition > 0.0 && startPosition < 1.0 && info.frames > 0) {
        const int64_t targetFrame = static_cast<int64_t>(startPosition * info.frames);
        m_reader->seek(targetFrame, SEEK_SET);
        m_playedFrames = targetFrame;
        m_playbackPosition = startPosition;
    }

    m_stopThread = false;
    m_diskReadThread = std::thread(&AudioFileStreamer::diskReadLoop, this);
}

void AudioFileStreamer::stop()
{
    m_stopThread = true;
    if (m_diskReadThread.joinable()) {
        m_diskReadThread.join();
    }

    if (m_reader && m_reader->isOpen()) {
        m_reader->close();
    }
}

size_t AudioFileStreamer::pop(int32_t * data, size_t count)
{
    const size_t read = m_ringBuffer.pop(data, count);

    const auto info = m_reader->info();
    if (read > 0) {
        if (info.channels > 0) {
            m_playedFrames += read / info.channels;
        }
    }

    if (info.frames > 0) {
        m_playbackPosition = static_cast<double>(m_playedFrames.load()) / info.frames;
        if (m_playedFrames >= static_cast<uint64_t>(info.frames)) {
            m_isFinished = true;
        }
    }

    return read;
}

void AudioFileStreamer::setPosition(double position)
{
    m_playbackPosition = position;
    const auto info = m_reader->info();
    if (m_reader->isOpen() && info.frames > 0) {
        const int64_t targetFrame = static_cast<int64_t>(position * info.frames);
        m_reader->seek(targetFrame, SEEK_SET);
        m_ringBuffer.clear();
        m_playedFrames = targetFrame;
        m_isFinished = targetFrame >= info.frames;
    }
}

double AudioFileStreamer::position() const
{
    return m_playbackPosition.load();
}

bool AudioFileStreamer::isFinished() const
{
    return m_isFinished.load();
}

int AudioFileStreamer::channels() const
{
    return m_reader->info().channels;
}

int AudioFileStreamer::sampleRate() const
{
    return m_reader->info().samplerate;
}

int64_t AudioFileStreamer::frames() const
{
    return m_reader->info().frames;
}

void AudioFileStreamer::diskReadLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopThread) {
        const size_t available = m_ringBuffer.writeAvailable();
        if (available > 0 && m_reader->isOpen()) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const auto info = m_reader->info();
            const int channels = info.channels;
            if (channels > 0) {
                const int64_t read = m_reader->readInt({ tempBuffer.data(), toRead });
                if (read > 0) {
                    m_ringBuffer.push(tempBuffer.data(), read * channels);
                } else {
                    // EOF or error
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace noteahead
