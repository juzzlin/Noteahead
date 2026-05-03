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

#ifndef AUDIO_PLAYER_RT_AUDIO_HPP
#define AUDIO_PLAYER_RT_AUDIO_HPP

#include "../../audio_file_streamer.hpp"
#include "../../audio_player.hpp"

#include <RtAudio.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace noteahead {

class AudioPlayerRtAudio : public AudioPlayer
{
public:
    explicit AudioPlayerRtAudio(AudioEngineS audioEngine = nullptr, RtAudio::Api api = RtAudio::UNSPECIFIED);
    ~AudioPlayerRtAudio() override;

    void start(const std::string & fileName, uint32_t bufferSize) override;
    void stop() override;

    std::vector<AudioDevice> getOutputDevices() override;
    void setOutputDevice(uint32_t deviceId) override;

    void setPosition(double position) override;
    double position() const override;

    bool isFinished() const override;

    uint32_t sampleRate() override;

private:
    static int playCallback(void * outputBuffer, void * inputBuffer,
                            uint32_t frameCount, double streamTime,
                            RtAudioStreamStatus status, void * userData);

    uint32_t initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize);

    RtAudio m_rtAudio;

    std::atomic_bool m_running = false;
    std::atomic<uint32_t> m_outputDeviceId = 0;

    AudioFileStreamer m_streamer;
};

} // namespace noteahead

#endif // AUDIO_PLAYER_RT_AUDIO_HPP
