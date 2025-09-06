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

#include <RtAudio.h>
#include <sndfile.h>

#include <atomic>
#include <string>

namespace noteahead {

class AudioRecorderRtAudio : public AudioRecorder
{
public:
    AudioRecorderRtAudio();
    ~AudioRecorderRtAudio() override;

    void start(const std::string & fileName) override;
    void stop() override;

    bool isRunning() const;

private:
    static int recordCallback(void * outputBuffer, void * inputBuffer,
                              unsigned int nFrames, double streamTime,
                              RtAudioStreamStatus status, void * userData);

    void initializeSoundFile(const std::string & fileName, unsigned int sampleRate, unsigned int channelCount);

    void initializeSoundStream(unsigned int deviceId, unsigned int channelCount, unsigned int sampleRate);

    RtAudio m_rtAudio;

    SNDFILE * m_sndFile = nullptr;

    SF_INFO m_sfInfo = {};

    std::atomic_bool m_running = false;
};

} // namespace noteahead

#endif // AUDIO_RECORDER_RT_AUDIO_HPP
