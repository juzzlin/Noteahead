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

#ifndef AUDIO_RECORDER_JACK_HPP
#define AUDIO_RECORDER_JACK_HPP

#include "../../audio_recorder.hpp"
#include <memory>

namespace noteahead {

class JackService;

class AudioRecorderJack : public AudioRecorder
{
public:
    using JackServiceS = std::shared_ptr<JackService>;
    explicit AudioRecorderJack(JackServiceS jackService);
    ~AudioRecorderJack() override;

    void start(const std::string & fileName, uint32_t bufferSize) override;
    void stop() override;

    std::vector<AudioDevice> getInputDevices() override;
    void setInputDevice(uint32_t deviceId) override;

    uint32_t sampleRate() override;

private:
    JackServiceS m_jackService;
};

} // namespace noteahead

#endif // AUDIO_RECORDER_JACK_HPP
