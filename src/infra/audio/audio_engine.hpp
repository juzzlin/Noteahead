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

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../../domain/utility/load_meter.hpp"

namespace noteahead {

class EffectRack;
class RealTimeWorkerPool;

struct AudioEngineWorkBuffer
{
    std::vector<double> deviceBuffer {};
    //! Holds the signal as it stood before the fader, for devices whose sends tap pre-fader.
    std::vector<double> preFaderBuffer {};
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

    //! Whole-callback DSP load, in percent of the real-time budget, plus an overrun count.
    LoadMeter & loadMeter();
    const LoadMeter & loadMeter() const;

    //! Oversampling factor used for realtime playback (offline export sets its own factor directly on
    //! the AudioContext). Read by the playback context builders and stamped onto each device's context.
    void setPlaybackOversampleFactor(uint8_t factor);
    uint8_t playbackOversampleFactor() const;

    EffectRack & sendEffectRack();
    EffectRack & insertEffectRack();

private:
    void ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize);
    void ensureEffectWetBuffers(size_t effectCount, uint32_t bufferSize);
    void ensureEffectActiveFlags(size_t effectCount);
    void ensureDeviceActiveFlags(size_t deviceCount);
    void ensureDeviceOutputBuffers(uint32_t bufferSize);

    void rebuildProcessingGraph();
    //! Marks which devices still feed the master directly. A device claimed as a SubMixer member
    //! is heard through that SubMixer instead, so its direct contribution has to be suppressed.
    void updateDirectOutSnapshot();
    //! Builds a signature of the current graph inputs (device slots + their sidechain deps) into a
    //! reusable buffer and reports whether it changed since the last rebuild. Allocation-free in
    //! steady state so it is safe to call every audio callback.
    bool processingGraphChanged();

    std::map<size_t, DeviceS> m_devices;
    std::unique_ptr<EffectRack> m_sendEffectRack;
    std::unique_ptr<EffectRack> m_insertEffectRack;

    // Cached snapshot of the send effect rack, refreshed only when the rack changes so that process()
    // does not copy the effect vector (and bump shared_ptr refcounts) on every audio callback.
    std::vector<std::shared_ptr<Effect>> m_sendEffectsSnapshot;
    uint64_t m_sendEffectsVersion = std::numeric_limits<uint64_t>::max();
    std::unique_ptr<RealTimeWorkerPool> m_workerPool;
    std::vector<AudioEngineWorkBuffer> m_workBuffers;
    std::vector<DeviceS> m_deviceSnapshot;
    std::vector<size_t> m_deviceSlotSnapshot;
    std::vector<uint8_t> m_deviceActiveFlags;
    std::vector<double> m_deviceSendSnapshot;
    std::vector<uint8_t> m_deviceDirectOutSnapshot;
    std::vector<std::vector<double>> m_sendBusBuffers;
    std::vector<std::vector<double>> m_effectWetBuffers;
    std::vector<uint8_t> m_effectActiveFlags;
    std::vector<std::vector<double>> m_deviceOutputBuffers;
    std::vector<std::span<const double>> m_deviceOutputBufferSpans;
    std::vector<std::vector<size_t>> m_processingLayers;
    std::vector<size_t> m_scratchDeps;
    std::vector<size_t> m_graphSignature;
    std::vector<size_t> m_prevGraphSignature;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isExclusive { false };
    LoadMeter m_loadMeter;
    std::atomic<uint8_t> m_playbackOversampleFactor { 2 };
};

} // namespace noteahead

#endif // AUDIO_ENGINE_HPP
