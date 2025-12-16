// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef AUDIO_RECORDER_RT_AUDIO_HPP
#define AUDIO_RECORDER_RT_AUDIO_HPP

#include "../../audio_recorder.hpp"
#include "../../ring_buffer.hpp"

#include <RtAudio.h>
#include <sndfile.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace noteahead {

class AudioRecorderRtAudio : public AudioRecorder
{
public:
    AudioRecorderRtAudio();
    ~AudioRecorderRtAudio() override;

    void start(const std::string & fileName, uint32_t bufferSize) override;
    void stop() override;

    bool isRunning() const;

private:
    static int recordCallback(void * outputBuffer, void * inputBuffer,
                              uint32_t nFrames, double streamTime,
                              RtAudioStreamStatus status, void * userData);

    void initializeSoundFile(const std::string & fileName, uint32_t sampleRate, uint32_t channelCount);
    void initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize);
    void diskWriteLoop();

    RtAudio m_rtAudio;

    SNDFILE * m_sndFile = nullptr;

    SF_INFO m_sfInfo = {};

    std::atomic_bool m_running = false;

    // Double buffering / Ring Buffer
    RingBuffer<int32_t> m_ringBuffer;
    std::thread m_diskWriteThread;
    std::atomic<bool> m_stopThread { false };
};

} // namespace noteahead

#endif // AUDIO_RECORDER_RT_AUDIO_HPP
