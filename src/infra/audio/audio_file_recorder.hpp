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

#ifndef AUDIO_FILE_RECORDER_HPP
#define AUDIO_FILE_RECORDER_HPP

#include "ring_buffer.hpp"
#include "backend/audio_file_reader.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace noteahead {

class AudioFileRecorder
{
public:
    explicit AudioFileRecorder(std::unique_ptr<AudioFileReader> writer = nullptr);
    ~AudioFileRecorder();

    void start(const std::string & fileName, uint32_t sampleRate, uint32_t channelCount, size_t bufferSize);
    void stop();

    bool push(const int32_t * data, size_t count);

private:
    void diskWriteLoop();

    RingBuffer<int32_t> m_ringBuffer;
    std::thread m_diskWriteThread;
    std::atomic<bool> m_stopThread { false };

    std::unique_ptr<AudioFileReader> m_writer;
};

} // namespace noteahead

#endif // AUDIO_FILE_RECORDER_HPP
