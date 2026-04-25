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

#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "../../domain/devices/device.hpp"

#include <memory>
#include <map>
#include <mutex>
#include <string>

namespace noteahead {

class AudioEngine
{
public:
    using DeviceS = std::shared_ptr<Device>;

    AudioEngine();
    ~AudioEngine();

    void addDevice(DeviceS device);
    void removeDevice(const std::string & name);
    DeviceS device(const std::string & name) const;

    void process(float * output, uint32_t nFrames, uint32_t sampleRate);

private:
    std::map<std::string, DeviceS> m_devices;
    mutable std::mutex m_mutex;
};

} // namespace noteahead

#endif // AUDIO_ENGINE_HPP
