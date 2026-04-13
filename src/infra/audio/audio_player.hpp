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

#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "audio_recorder.hpp"

namespace noteahead {

class AudioPlayer
{
public:
    explicit AudioPlayer();
    virtual ~AudioPlayer();

    virtual void start(const std::string & fileName, uint32_t bufferSize);
    virtual void stop();

    virtual std::vector<AudioDevice> getOutputDevices();
    virtual void setOutputDevice(uint32_t deviceId);

    virtual void setPosition(double position);
    virtual double position() const;

    virtual bool isFinished() const;

    virtual uint32_t sampleRate();
};

} // namespace noteahead

#endif // AUDIO_PLAYER_HPP
