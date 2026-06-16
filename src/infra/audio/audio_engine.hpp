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

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace noteahead {

class EffectRack;
class RealTimeWorkerPool;

struct AudioEngineWorkBuffer
{
    std::vector<double> deviceBuffer {};
    std::vector<double> outputBuffer {};
    std::vector<std::vector<double>> sendBuffers {};
};

class AudioEngine
{
public:
    using DeviceS = std::shared_ptr<Device>;

    AudioEngine();
    ~AudioEngine();

    void setDevice(size_t slotIndex, DeviceS device);
    void clearDevice(size_t slotIndex);
    DeviceS device(size_t slotIndex) const;
    DeviceS device(const std::string & name) const;

    using DeviceNames = std::vector<std::string>;
    DeviceNames deviceNames() const;

    void setBpm(float bpm);

    void process(AudioContext & context);

    void reset();
    void clear();

    void setIsExclusive(bool exclusive);
    bool isExclusive() const;

    EffectRack & sendEffectRack();
    EffectRack & insertEffectRack();

private:
    void ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize);
    void ensureEffectWetBuffers(size_t effectCount, uint32_t bufferSize);
    void ensureEffectActiveFlags(size_t effectCount);
    void ensureDeviceActiveFlags(size_t deviceCount);
    void ensureDeviceOutputBuffers(uint32_t bufferSize);

    void rebuildProcessingGraph();

    std::map<size_t, DeviceS> m_devices;
    std::unique_ptr<EffectRack> m_sendEffectRack;
    std::unique_ptr<EffectRack> m_insertEffectRack;
    std::unique_ptr<RealTimeWorkerPool> m_workerPool;
    std::vector<AudioEngineWorkBuffer> m_workBuffers;
    std::vector<DeviceS> m_deviceSnapshot;
    std::vector<size_t> m_deviceSlotSnapshot;
    std::vector<uint8_t> m_deviceActiveFlags;
    std::vector<double> m_deviceSendSnapshot;
    std::vector<std::vector<double>> m_sendBusBuffers;
    std::vector<std::vector<double>> m_effectWetBuffers;
    std::vector<uint8_t> m_effectActiveFlags;
    std::vector<std::vector<double>> m_deviceOutputBuffers;
    std::vector<std::span<const double>> m_deviceOutputBufferSpans;
    std::vector<std::vector<size_t>> m_processingLayers;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isExclusive { false };
};

} // namespace noteahead

#endif // AUDIO_ENGINE_HPP
