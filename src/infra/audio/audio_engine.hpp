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
#include "../../domain/devices/effect_rack.hpp"
#include "real_time_worker_pool.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace noteahead {

struct AudioEngineWorkBuffer
{
    std::vector<float> deviceBuffer {};
    std::vector<float> outputBuffer {};
    std::vector<std::vector<float>> sendBuffers {};
};

class AudioEngine
{
public:
    using DeviceS = std::shared_ptr<Device>;

    AudioEngine();
    ~AudioEngine();

    void addDevice(DeviceS device);
    void removeDevice(const std::string & name);
    DeviceS device(const std::string & name) const;
    using DeviceNames = std::vector<std::string>;
    DeviceNames deviceNames() const;

    void setBpm(float bpm);

    void process(float * output, uint32_t frameCount, uint32_t sampleRate);

    void reset();

    void setIsExclusive(bool exclusive);
    bool isExclusive() const;

    EffectRack & effectRack();

private:
    void ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize);
    void ensureEffectWetBuffers(size_t effectCount, uint32_t bufferSize);

    std::map<std::string, DeviceS> m_devices {};
    EffectRack m_effectRack {};
    RealTimeWorkerPool m_workerPool {};
    std::vector<AudioEngineWorkBuffer> m_workBuffers {};
    std::vector<DeviceS> m_deviceSnapshot {};
    std::vector<float> m_deviceSendSnapshot {};
    std::vector<std::vector<float>> m_sendBusBuffers {};
    std::vector<std::vector<float>> m_effectWetBuffers {};
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isExclusive { false };
};

} // namespace noteahead

#endif // AUDIO_ENGINE_HPP
