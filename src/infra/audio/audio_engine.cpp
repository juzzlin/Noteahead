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

#include "audio_engine.hpp"

namespace noteahead {

AudioEngine::AudioEngine() = default;
AudioEngine::~AudioEngine() = default;

void AudioEngine::addDevice(DeviceS device)
{
    if (!device) {
        return;
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_devices[device->name()] = std::move(device);
}

void AudioEngine::removeDevice(const std::string & name)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_devices.erase(name);
}

AudioEngine::DeviceS AudioEngine::device(const std::string & name) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (auto it { m_devices.find(name) }; it != m_devices.end()) {
        return it->second;
    }
    return nullptr;
}

void AudioEngine::process(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & [name, device] : m_devices) {
        device->processAudio(output, nFrames, sampleRate);
    }
}

} // namespace noteahead
