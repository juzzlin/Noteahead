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

#include "../../common/constants.hpp"
#include "../../common/denormal_protection.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/reverb.hpp"
#include "real_time_worker_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <pthread.h>

namespace noteahead {

namespace {

constexpr auto TAG = "AudioEngine";

//! Modest real-time priority for the playback workers: above ordinary threads, below the audio
//! callback itself, which must always be able to preempt them.

struct DeviceProcessContext
{
    std::vector<AudioEngine::DeviceS> * devices {};
    std::vector<AudioEngineWorkBuffer> * workBuffers {};
    std::vector<uint8_t> * deviceActiveFlags {};
    std::vector<double> * deviceSends {};
    //! Per snapshot index: 0 when a SubMixer claims this device, so it must not also reach
    //! the master or the global sends. Its output buffer is still written for the SubMixer.
    std::vector<uint8_t> * deviceDirectOut {};
    std::vector<size_t> * layerDevices {};
    std::vector<size_t> * slotSnapshot {};
    std::vector<std::vector<double>> * deviceOutputBuffersMutable {};
    std::span<const std::span<const double>> deviceOutputBuffers {};
    size_t sendCount {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
    uint32_t bufferSize {};
    double bpm {};
    uint8_t oversampleFactor {};
    bool offline {};
};

struct EffectProcessContext
{
    std::vector<EffectRack::EffectS> * effects {};
    std::vector<std::vector<double>> * sendBusBuffers {};
    std::vector<std::vector<double>> * effectWetBuffers {};
    std::vector<uint8_t> * effectActiveFlags {};
    std::vector<uint8_t> * sendBusHasSignal {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
    double bpm {};
    uint8_t oversampleFactor {};
    bool offline {};
};

bool bufferContainsSignal(const std::vector<double> & buffer, uint32_t bufferSize)
{
    constexpr double threshold = 1.0e-12;
    for (uint32_t i = 0; i < bufferSize; i++) {
        if (std::abs(buffer[i]) > threshold) {
            return true;
        }
    }
    return false;
}

void processDeviceTask(void * context, size_t taskIndex, size_t workerIndex)
{
    auto & deviceContext = *static_cast<DeviceProcessContext *>(context);
    const auto deviceSnapshotIndex = deviceContext.layerDevices ? deviceContext.layerDevices->at(taskIndex) : taskIndex;
    auto & device = deviceContext.devices->at(deviceSnapshotIndex);
    auto & workBuffer = deviceContext.workBuffers->at(workerIndex);

    const double bufferSeconds = static_cast<double>(deviceContext.frameCount) / deviceContext.sampleRate;

    std::fill(workBuffer.deviceBuffer.begin(), workBuffer.deviceBuffer.begin() + deviceContext.bufferSize, 0.0);

    // Going silent is not on its own enough to stop processing a device: an insert whose gain
    // envelope follows a detector -- a ducker, a side-chained compressor -- would then freeze
    // wherever the last block left it, and the device's next note would start on that stale amount
    // of gain reduction instead of on what the detector says. How far off it lands depends on where
    // the freeze fell on the detector's decay, which depends on the block size, which is why a
    // render (one tick per block) and real-time playback (a whole audio buffer per block) disagree
    // about it. Such an insert reports itself unsettled until it has released, and the device keeps
    // running until then; everything else settles immediately and is skipped exactly as before.
    if (!device->hasActiveAudio() && !deviceContext.deviceActiveFlags->at(deviceSnapshotIndex) && device->insertEffectsSettled()) {
        if (deviceContext.deviceOutputBuffersMutable) {
            const auto slotIndex = deviceContext.slotSnapshot->at(deviceSnapshotIndex);
            auto & outputBuffer = deviceContext.deviceOutputBuffersMutable->at(slotIndex);
            std::fill(outputBuffer.begin(), outputBuffer.begin() + deviceContext.bufferSize, 0.0);
        }
        // A skipped device produces nothing and costs nothing, and has to say so: without these
        // its meters would keep reading whatever they last showed, for as long as it stays silent.
        // The device buffer was just cleared, so it is the silence to report.
        device->meter().write(workBuffer.deviceBuffer.data(), deviceContext.frameCount, deviceContext.sampleRate);
        device->loadMeter().addBlock(std::chrono::nanoseconds::zero(), bufferSeconds);
        return;
    }

    AudioContext audioContext { std::span(workBuffer.deviceBuffer.data(), deviceContext.bufferSize), deviceContext.frameCount, deviceContext.sampleRate, deviceContext.bpm, deviceContext.deviceOutputBuffers, deviceContext.oversampleFactor, deviceContext.offline };

    // Cheap enough to read unconditionally; the meter itself is a no-op while nothing is displayed.
    const auto processingStarted = std::chrono::steady_clock::now();

    device->processAudio(audioContext);

    // Level tap for gain staging: post-gain and pre-insert, the level the Gain knob is set against.
    device->meter().write(workBuffer.deviceBuffer.data(), deviceContext.frameCount, deviceContext.sampleRate);

    // Where the fader sits relative to the insert rack is per-device. Pre-inserts is how devices
    // behaved before the setting existed and stays the default; post-inserts is the gain-staging
    // arrangement, where riding the fader can no longer change how hard the inserts are driven.
    const bool preFaderSend = device->sendTap() == Device::SendTap::PreFader && deviceContext.sendCount;
    const auto capturePreFader = [&] {
        if (preFaderSend) {
            std::copy(workBuffer.deviceBuffer.begin(), workBuffer.deviceBuffer.begin() + deviceContext.bufferSize, workBuffer.preFaderBuffer.begin());
        }
    };

    if (device->faderPosition() == Device::FaderPosition::PreInserts) {
        capturePreFader();
        device->applyFader(audioContext);
        device->processInsertEffects(audioContext);
    } else {
        device->processInsertEffects(audioContext);
        capturePreFader();
        device->applyFader(audioContext);
    }

    device->loadMeter().addBlock(std::chrono::steady_clock::now() - processingStarted, bufferSeconds);

    // Feed this device's final output to its oscilloscope tap (no-op unless a scope is active).
    device->scope().write(workBuffer.deviceBuffer.data(), deviceContext.frameCount, deviceContext.sampleRate);

    // Clipping is judged on what the device finally hands over, so it accounts for the fader, any
    // boost past unity and whatever the insert rack did.
    device->clipDetector().write(workBuffer.deviceBuffer.data(), deviceContext.frameCount);

    if (deviceContext.deviceOutputBuffersMutable) {
        const auto slotIndex = deviceContext.slotSnapshot->at(deviceSnapshotIndex);
        auto & outputBuffer = deviceContext.deviceOutputBuffersMutable->at(slotIndex);
        std::copy(workBuffer.deviceBuffer.begin(), workBuffer.deviceBuffer.begin() + deviceContext.bufferSize, outputBuffer.begin());
    }

    const bool hasOutputSignal = bufferContainsSignal(workBuffer.deviceBuffer, deviceContext.bufferSize);
    deviceContext.deviceActiveFlags->at(deviceSnapshotIndex) = hasOutputSignal ? 1 : 0;

    // A device claimed by a SubMixer is heard only through that SubMixer, which sums the output
    // buffer written above. Letting its dry path also reach the master here would play the group
    // twice: once dry, once through the SubMixer's effects.
    //
    // Its sends still apply. A send bus is a parallel tap rather than part of the master sum, so
    // nothing is double-counted by keeping them, and a device keeps whatever reverb amount it was
    // given instead of losing it silently the moment it joins a group.
    const bool directOut = deviceContext.deviceDirectOut->at(deviceSnapshotIndex) != 0;
    if (!directOut && !deviceContext.sendCount) {
        return;
    }

    for (uint32_t i = 0; i < deviceContext.bufferSize; i++) {
        const double sample = workBuffer.deviceBuffer[i];
        if (directOut) {
            workBuffer.outputBuffer[i] += sample;
        }

        // A pre-fader send keeps its level when the fader moves, so it reads the captured buffer
        // rather than the one the fader has already scaled.
        const double sendSample = preFaderSend ? workBuffer.preFaderBuffer[i] : sample;
        for (size_t sendIndex = 0; sendIndex < deviceContext.sendCount; sendIndex++) {
            const double send = deviceContext.deviceSends->at(deviceSnapshotIndex * deviceContext.sendCount + sendIndex);
            workBuffer.sendBuffers[sendIndex][i] += sendSample * send;
        }
    }
}

void processEffectTask(void * context, size_t taskIndex, size_t /*workerIndex*/)
{
    auto & effectContext = *static_cast<EffectProcessContext *>(context);
    auto & effect = effectContext.effects->at(taskIndex);
    const auto & sendBus = effectContext.sendBusBuffers->at(taskIndex);
    auto & wetBuffer = effectContext.effectWetBuffers->at(taskIndex);
    const auto bufferSize = effectContext.frameCount * 2;

    if (!effect) {
        std::fill(wetBuffer.begin(), wetBuffer.begin() + bufferSize, 0.0);
        effectContext.effectActiveFlags->at(taskIndex) = 0;
        return;
    }

    if (!effectContext.sendBusHasSignal->at(taskIndex) && !effectContext.effectActiveFlags->at(taskIndex)) {
        std::fill(wetBuffer.begin(), wetBuffer.begin() + bufferSize, 0.0);
        return;
    }

    effect->setSampleRate(effectContext.sampleRate);
    // This is a send bus, so the effect has to keep the dry whole: the return below is the
    // difference against what the bus handed over.
    effect->setSendMode(true);

    // Copy dry signal to wet buffer for in-place processing
    std::copy(sendBus.begin(), sendBus.begin() + bufferSize, wetBuffer.begin());

    AudioContext context_obj { std::span(wetBuffer.data(), bufferSize), effectContext.frameCount, effectContext.sampleRate, effectContext.bpm, {}, effectContext.oversampleFactor, effectContext.offline };
    effect->process(context_obj);

    bool hasWetSignal = false;
    for (uint32_t i = 0; i < bufferSize; i++) {
        wetBuffer[i] -= sendBus[i];
        if (std::abs(wetBuffer[i]) > 1.0e-12) {
            hasWetSignal = true;
        }
    }

    effectContext.effectActiveFlags->at(taskIndex) = hasWetSignal ? 1 : 0;
}

} // namespace

void AudioEngine::ensureDeviceOutputBuffers(uint32_t bufferSize)
{
    const auto deviceCount = Constants::deviceRackSize();
    if (m_deviceOutputBuffers.size() != deviceCount) {
        m_deviceOutputBuffers.resize(deviceCount);
        m_deviceOutputBufferSpans.resize(deviceCount);
    }

    for (size_t i = 0; i < deviceCount; i++) {
        if (m_deviceOutputBuffers[i].size() != bufferSize) {
            m_deviceOutputBuffers[i].assign(bufferSize, 0.0);
        }
        m_deviceOutputBufferSpans[i] = std::span<const double> { m_deviceOutputBuffers[i].data(), bufferSize };
    }
}

bool AudioEngine::processingGraphChanged()
{
    // Flatten the graph inputs into a signature: [deviceCount, (slot, depCount, deps...) per device].
    // Built into reusable buffers so no allocation happens once they have grown to size.
    m_graphSignature.clear();
    m_graphSignature.push_back(m_deviceSnapshot.size());
    for (size_t i = 0; i < m_deviceSnapshot.size(); i++) {
        m_graphSignature.push_back(m_deviceSlotSnapshot[i]);
        m_deviceSnapshot[i]->sidechainDependencies(m_scratchDeps);
        m_graphSignature.push_back(m_scratchDeps.size());
        for (const auto dep : m_scratchDeps) {
            m_graphSignature.push_back(dep);
        }
    }

    if (m_graphSignature == m_prevGraphSignature) {
        return false;
    }
    std::swap(m_prevGraphSignature, m_graphSignature);
    return true;
}

void AudioEngine::rebuildProcessingGraph()
{
    // The topology only changes when devices are added/removed or their sidechain routing changes,
    // which is rare. Skip the (allocating) rebuild and reuse the cached layers otherwise, so the
    // audio callback does no heap allocation in steady state.
    if (!processingGraphChanged()) {
        return;
    }

    // Membership shows up in the graph signature (members are sidechain dependencies), so the
    // claimed-slot map only has to be recomputed when we already know the topology moved.
    updateDirectOutSnapshot();

    m_processingLayers.clear();
    if (m_deviceSnapshot.empty()) {
        return;
    }

    const auto deviceCount = m_deviceSnapshot.size();
    std::vector<std::vector<size_t>> adj(deviceCount);
    std::vector<int> inDegree(deviceCount, 0);

    for (size_t i = 0; i < deviceCount; i++) {
        m_deviceSnapshot[i]->sidechainDependencies(m_scratchDeps);
        for (const auto slotIndex : m_scratchDeps) {
            for (size_t j = 0; j < deviceCount; j++) {
                if (m_deviceSlotSnapshot[j] == slotIndex) {
                    adj[j].push_back(i);
                    inDegree[i]++;
                    break;
                }
            }
        }
    }

    std::vector<size_t> currentLayer;
    std::vector<bool> processed(deviceCount, false);
    for (size_t i = 0; i < deviceCount; i++) {
        if (inDegree[i] == 0) {
            currentLayer.push_back(i);
            processed[i] = true;
        }
    }

    while (!currentLayer.empty()) {
        m_processingLayers.push_back(currentLayer);
        std::vector<size_t> nextLayer;
        for (const auto u : currentLayer) {
            for (const auto v : adj[u]) {
                if (--inDegree[v] == 0) {
                    nextLayer.push_back(v);
                    processed[v] = true;
                }
            }
        }
        currentLayer = std::move(nextLayer);
    }

    // Handle circular dependencies by adding remaining unprocessed devices to a final layer
    std::vector<size_t> circularLayer;
    for (size_t i = 0; i < deviceCount; i++) {
        if (!processed[i]) {
            circularLayer.push_back(i);
        }
    }
    if (!circularLayer.empty()) {
        m_processingLayers.push_back(circularLayer);
    }
}

void AudioEngine::updateDirectOutSnapshot()
{
    m_deviceDirectOutSnapshot.assign(m_deviceSnapshot.size(), 1);

    for (const auto & device : m_deviceSnapshot) {
        for (const auto claimedSlot : device->claimedOutputSlots()) {
            for (size_t i = 0; i < m_deviceSlotSnapshot.size(); i++) {
                if (m_deviceSlotSnapshot[i] == claimedSlot) {
                    m_deviceDirectOutSnapshot[i] = 0;
                    break;
                }
            }
        }
    }
}

AudioEngine::AudioEngine()
  : m_sendEffectRack { std::make_unique<EffectRack>() }
  , m_insertEffectRack { std::make_unique<EffectRack>() }
  , m_workerPool { std::make_unique<RealTimeWorkerPool>() }
{
    enableHardwareDenormalProtection();
}

AudioEngine::~AudioEngine() = default;

EffectRack & AudioEngine::sendEffectRack()
{
    return *m_sendEffectRack;
}

EffectRack & AudioEngine::insertEffectRack()
{
    return *m_insertEffectRack;
}

void AudioEngine::setDevice(size_t slotIndex, DeviceS device)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_devices[slotIndex] = std::move(device);
}

void AudioEngine::clearDevice(size_t slotIndex)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_devices.erase(slotIndex);
}

AudioEngine::DeviceS AudioEngine::device(size_t slotIndex) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (const auto it = m_devices.find(slotIndex); it != m_devices.end()) {
        return it->second;
    }
    return nullptr;
}

AudioEngine::DeviceS AudioEngine::device(const std::string & name) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    if (name.starts_with(prefix)) {
        try {
            const auto slotStr = name.substr(prefix.length() + 1);
            const auto slotIndex = std::stoul(slotStr) - 1;
            if (const auto it = m_devices.find(slotIndex); it != m_devices.end()) {
                return it->second;
            }
        } catch (...) {
        }
    }

    for (auto const & [index, device] : m_devices) {
        if (device && device->name() == name) {
            return device;
        }
    }

    return nullptr;
}

AudioEngine::DeviceNames AudioEngine::deviceNames() const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    DeviceNames names;
    for (auto const & [index, device] : m_devices) {
        names.push_back(Constants::internalDevicePortPrefix().toStdString() + " " + std::to_string(index + 1));
    }
    return names;
}

void AudioEngine::setBpm(float bpm)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_sendEffectRack->setBpm(bpm);
    m_insertEffectRack->setBpm(bpm);
    for (auto const & [index, device] : m_devices) {
        if (device) {
            device->setBpm(bpm);
        }
    }
}

void AudioEngine::process(AudioContext & context)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    const auto callbackStarted = std::chrono::steady_clock::now();

    const uint32_t bufferSize = context.frameCount * 2;
    if (!bufferSize) {
        return;
    }
    // Refresh the cached send-effects snapshot only when the rack actually changed, so the common
    // case avoids copying the vector (and bumping shared_ptr refcounts) under a lock every callback.
    if (const auto version = m_sendEffectRack->version(); version != m_sendEffectsVersion) {
        m_sendEffectsSnapshot = m_sendEffectRack->effects();
        m_sendEffectsVersion = version;
    }
    auto & effects = m_sendEffectsSnapshot;
    const size_t sendCount = effects.size();
    const size_t laneCount = m_workerPool->laneCount();

    if (!m_isExclusive.load()) {
        detectCallbackScheduling();
    }

    // Offline rendering fans out its send effects: it has no deadline and only gains from the
    // throughput. Its devices deliberately do not -- see fanOutDevices below.
    // Real-time playback does so only when asked to, when the workers hold real-time scheduling,
    // and when the thread driving playback is itself real-time. That last condition matters as much
    // as the others: real-time workers above an ordinary callback thread preempt the very thread
    // waiting on them, which is heard as stuttering. Some backends — PulseAudio through RtAudio in
    // particular — do not give their callback thread real-time scheduling at all.
    const bool useWorkers = m_isExclusive.load() || (m_playbackThreadingEnabled.load() && callbackIsRealTime() && m_workerPool->hasRealTimeScheduling());

    ensureWorkBuffers(laneCount, sendCount, bufferSize);
    ensureEffectWetBuffers(sendCount, bufferSize);
    ensureEffectActiveFlags(sendCount);

    if (m_sendBusBuffers.size() != sendCount) {
        m_sendBusBuffers.resize(sendCount);
    }

    for (auto & bus : m_sendBusBuffers) {
        if (bus.size() < bufferSize) {
            bus.resize(bufferSize, 0.0);
        } else {
            std::fill(bus.begin(), bus.begin() + bufferSize, 0.0);
        }
    }

    m_deviceSnapshot.clear();
    m_deviceSlotSnapshot.clear();
    if (!m_devices.empty()) {
        m_deviceSnapshot.reserve(m_devices.size());
        m_deviceSlotSnapshot.reserve(m_devices.size());
        for (auto const & [index, device] : m_devices) {
            if (device) {
                m_deviceSnapshot.push_back(device);
                m_deviceSlotSnapshot.push_back(index);
            }
        }
    }

    if (!m_deviceSnapshot.empty()) {
        ensureDeviceActiveFlags(m_deviceSnapshot.size());
        ensureDeviceOutputBuffers(bufferSize);
        rebuildProcessingGraph();

        m_deviceSendSnapshot.resize(m_deviceSnapshot.size() * sendCount);
        for (size_t deviceIndex = 0; deviceIndex < m_deviceSnapshot.size(); deviceIndex++) {
            for (size_t sendIndex = 0; sendIndex < sendCount; sendIndex++) {
                m_deviceSendSnapshot[deviceIndex * sendCount + sendIndex] = static_cast<double>(m_deviceSnapshot[deviceIndex]->reverbSend(sendIndex));
            }
        }

        // Recomputed only when the topology changes; this is just a cheap safety guard so the
        // per-device lookup below can never index out of range on the audio thread.
        if (m_deviceDirectOutSnapshot.size() != m_deviceSnapshot.size()) {
            m_deviceDirectOutSnapshot.assign(m_deviceSnapshot.size(), 1);
        }

        // Fanning out costs the same whether or not the workers find anything to do: they are woken
        // and have to be waited for either way, and the handshake spins on both sides. With at most
        // one device actually producing audio there is nothing to overlap, so the work is cheaper
        // done in place. That is also what keeps the workers off the CPU between songs: a stopped
        // song leaves every device silent, and the callback keeps running to stay ready.
        size_t activeDeviceCount = 0;
        for (size_t i = 0; i < m_deviceSnapshot.size(); i++) {
            if (m_deviceSnapshot[i]->hasActiveAudio() || m_deviceActiveFlags[i]) {
                activeDeviceCount++;
            }
        }
        // Devices are summed per worker lane, and which lane a device lands in is decided by
        // whichever worker wins the task queue -- so the grouping of the sum changes run to run.
        // Float addition is not associative, so that alone makes a render irreproducible. Offline
        // therefore runs the devices in order on one lane: a render has no deadline, and the
        // throughput is worth less than being able to render the same song twice and get the same
        // file. Send effects are unaffected -- each writes its own task-indexed buffer -- so they
        // still fan out below.
        const bool fanOutDevices = useWorkers && activeDeviceCount > 1 && !context.offline;

        // Serial processing uses only lane 0, so clear/sum just that lane then; parallel rendering
        // spreads work across all lanes. The buffers stay allocated at the full lane count either way,
        // so no reallocation happens when switching between real-time playback and rendering.
        const size_t usedLanes = fanOutDevices ? m_workBuffers.size() : std::min<size_t>(1, m_workBuffers.size());

        for (size_t lane = 0; lane < usedLanes; lane++) {
            auto & workBuffer = m_workBuffers[lane];
            std::fill(workBuffer.outputBuffer.begin(), workBuffer.outputBuffer.begin() + bufferSize, 0.0);
            for (auto & sendBuffer : workBuffer.sendBuffers) {
                std::fill(sendBuffer.begin(), sendBuffer.begin() + bufferSize, 0.0);
            }
        }

        for (auto & layer : m_processingLayers) {
            DeviceProcessContext deviceContext {
                &m_deviceSnapshot,
                &m_workBuffers,
                &m_deviceActiveFlags,
                &m_deviceSendSnapshot,
                &m_deviceDirectOutSnapshot,
                &layer,
                &m_deviceSlotSnapshot,
                &m_deviceOutputBuffers,
                std::span<const std::span<const double>>(m_deviceOutputBufferSpans),
                sendCount,
                context.frameCount,
                context.sampleRate,
                bufferSize,
                context.bpm,
                context.oversampleFactor,
                context.offline
            };
            if (fanOutDevices) {
                m_workerPool->run(layer.size(), &deviceContext, processDeviceTask);
            } else {
                for (size_t taskIndex = 0; taskIndex < layer.size(); taskIndex++) {
                    processDeviceTask(&deviceContext, taskIndex, 0);
                }
            }
        }

        // Sum the (parallel) lane results into the main output and send buses.
        for (size_t lane = 0; lane < usedLanes; lane++) {
            const auto & workBuffer = m_workBuffers[lane];
            for (uint32_t i = 0; i < bufferSize; i++) {
                context.buffer[i] += workBuffer.outputBuffer[i];
            }
            for (size_t sendIndex = 0; sendIndex < sendCount; sendIndex++) {
                auto & sendBus = m_sendBusBuffers[sendIndex];
                const auto & laneSendBus = workBuffer.sendBuffers[sendIndex];
                for (uint32_t i = 0; i < bufferSize; i++) {
                    sendBus[i] += laneSendBus[i];
                }
            }
        }
    }

    if (m_sendEffectRack->enabled() && std::ranges::any_of(effects, [](const auto & effect) { return effect != nullptr; })) {
        if (m_sendBusHasSignal.size() != sendCount) {
            m_sendBusHasSignal.assign(sendCount, 0);
        }
        // A send whose bus is quiet and whose tail has run out is skipped, so the dispatch counts
        // only the ones that will really run. A reverb tail keeps its send alive well past the last
        // note, which is exactly when this matters.
        size_t activeSendCount = 0;
        for (size_t i = 0; i < sendCount; i++) {
            m_sendBusHasSignal[i] = bufferContainsSignal(m_sendBusBuffers[i], bufferSize) ? 1 : 0;
            if (effects[i] && (m_sendBusHasSignal[i] || m_effectActiveFlags[i])) {
                activeSendCount++;
            }
        }

        EffectProcessContext effectContext {
            &effects,
            &m_sendBusBuffers,
            &m_effectWetBuffers,
            &m_effectActiveFlags,
            &m_sendBusHasSignal,
            context.frameCount,
            context.sampleRate,
            context.bpm,
            context.oversampleFactor,
            context.offline
        };
        if (useWorkers && activeSendCount > 1) {
            m_workerPool->run(sendCount, &effectContext, processEffectTask);
        } else {
            for (size_t taskIndex = 0; taskIndex < sendCount; taskIndex++) {
                processEffectTask(&effectContext, taskIndex, 0);
            }
        }

        for (const auto & wetBuffer : m_effectWetBuffers) {
            for (uint32_t i = 0; i < bufferSize; i++) {
                context.buffer[i] += wetBuffer[i];
            }
        }
    } else {
        for (size_t i = 0; i < sendCount; i++) {
            if (m_effectActiveFlags[i]) {
                std::fill(m_effectWetBuffers[i].begin(), m_effectWetBuffers[i].begin() + bufferSize, 0.0);
                m_effectActiveFlags[i] = 0;
            }
        }
    }

    m_insertEffectRack->processInPlace(context);

    // Whole-callback load. Over 100% is what the listener hears as a dropout, so the meter counts
    // those separately.
    m_loadMeter.addBlock(std::chrono::steady_clock::now() - callbackStarted,
                         static_cast<double>(context.frameCount) / context.sampleRate);
}

LoadMeter & AudioEngine::loadMeter()
{
    return m_loadMeter;
}

const LoadMeter & AudioEngine::loadMeter() const
{
    return m_loadMeter;
}

void AudioEngine::reset()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & [index, device] : m_devices) {
        if (device) {
            device->resetAudio();
        }
    }
    m_sendEffectRack->reset();
    m_insertEffectRack->reset();

    std::fill(m_deviceActiveFlags.begin(), m_deviceActiveFlags.end(), 0);
    std::fill(m_effectActiveFlags.begin(), m_effectActiveFlags.end(), 0);
}

void AudioEngine::clear()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & [index, device] : m_devices) {
        if (device) {
            device->resetAudio();
        }
    }
    m_devices.clear();
    m_sendEffectRack->reset();
    m_insertEffectRack->reset();

    // Also clear the actual effects to ensure a fresh state for "New Project"
    for (size_t i = 0; i < m_sendEffectRack->effectCount(); i++) {
        m_sendEffectRack->setEffect(i, nullptr);
    }
    for (size_t i = 0; i < m_insertEffectRack->effectCount(); i++) {
        m_insertEffectRack->setEffect(i, nullptr);
    }

    std::fill(m_deviceActiveFlags.begin(), m_deviceActiveFlags.end(), 0);
    std::fill(m_effectActiveFlags.begin(), m_effectActiveFlags.end(), 0);
}

void AudioEngine::setIsExclusive(bool exclusive)
{
    m_isExclusive = exclusive;
}

void AudioEngine::detectCallbackScheduling()
{
    if (m_callbackPolicy.load(std::memory_order_acquire) >= 0) {
        return;
    }
    int policy = 0;
    sched_param parameters {};
    if (pthread_getschedparam(pthread_self(), &policy, &parameters) != 0) {
        return;
    }
    m_callbackPriority.store(parameters.sched_priority, std::memory_order_release);
    m_callbackPolicy.store(policy, std::memory_order_release);
    juzzlin::L(TAG).info() << "Playback thread scheduling: policy " << policy
                           << " (" << (policy == SCHED_FIFO ? "FIFO" : policy == SCHED_RR ? "RR"
                                                                                          : "not real-time")
                           << "), priority " << parameters.sched_priority;
}

bool AudioEngine::callbackIsRealTime() const
{
    const int policy = m_callbackPolicy.load(std::memory_order_acquire);
    return policy == SCHED_FIFO || policy == SCHED_RR;
}

void AudioEngine::prepare(uint32_t frameCount)
{
    const uint32_t bufferSize = frameCount * 2;
    if (!bufferSize) {
        return;
    }

    std::lock_guard<std::mutex> lock { m_mutex };

    const size_t sendCount = m_sendEffectRack->effectCount();
    ensureWorkBuffers(m_workerPool->laneCount(), sendCount, bufferSize);
    ensureEffectWetBuffers(sendCount, bufferSize);
    ensureEffectActiveFlags(sendCount);
    ensureDeviceOutputBuffers(bufferSize);

    if (m_sendBusBuffers.size() != sendCount) {
        m_sendBusBuffers.resize(sendCount);
    }
    for (auto & bus : m_sendBusBuffers) {
        if (bus.size() < bufferSize) {
            bus.resize(bufferSize, 0.0);
        }
    }

    juzzlin::L(TAG).info() << "Prepared engine buffers for " << frameCount << " frames";
}

void AudioEngine::setCallbackRealTimePriority(int priority)
{
    if (priority <= 0) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock { m_mutex };
        m_callbackPriority.store(priority, std::memory_order_release);
        m_callbackPolicy.store(SCHED_FIFO, std::memory_order_release);
    }
    juzzlin::L(TAG).info() << "Backend reported callback real-time priority " << priority;
    // Re-size the workers against it: they were started against a guess.
    applyWorkerPriority();
}

void AudioEngine::applyWorkerPriority()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    // Strictly below the thread driving playback, so the workers can never preempt the thread that
    // is waiting for them. Until that thread has been seen, assume the priority we ask the backend
    // for; a worker below the callback is safe either way, one above it stutters.
    const int callbackPriority = callbackIsRealTime() ? m_callbackPriority.load(std::memory_order_acquire)
                                                      : Constants::audioCallbackRealTimePriority();
    m_workerPool->setRealTimePriority(m_playbackThreadingEnabled.load() ? std::max(1, callbackPriority - 1) : 0);
}

void AudioEngine::setPlaybackThreadingEnabled(bool enabled)
{
    m_playbackThreadingEnabled = enabled;
    // Real-time scheduling is only asked for when playback needs it. Export does not: elevating
    // those workers would let a render preempt the UI and make the app unresponsive.
    applyWorkerPriority();
}

bool AudioEngine::playbackThreadingEnabled() const
{
    return m_playbackThreadingEnabled;
}

bool AudioEngine::supportsPlaybackThreading() const
{
    return callbackIsRealTime() && m_workerPool->hasRealTimeScheduling();
}

bool AudioEngine::isExclusive() const
{
    return m_isExclusive;
}

void AudioEngine::setPlaybackOversampleFactor(uint8_t factor)
{
    m_playbackOversampleFactor = factor;
}

uint8_t AudioEngine::playbackOversampleFactor() const
{
    return m_playbackOversampleFactor;
}

void AudioEngine::ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize)
{
    if (m_workBuffers.size() != laneCount) {
        m_workBuffers.resize(laneCount);
    }

    for (auto & workBuffer : m_workBuffers) {
        if (workBuffer.deviceBuffer.size() < bufferSize) {
            workBuffer.deviceBuffer.resize(bufferSize, 0.0);
        }
        if (workBuffer.preFaderBuffer.size() < bufferSize) {
            workBuffer.preFaderBuffer.resize(bufferSize, 0.0);
        }
        if (workBuffer.outputBuffer.size() < bufferSize) {
            workBuffer.outputBuffer.resize(bufferSize, 0.0);
        }
        if (workBuffer.sendBuffers.size() != sendCount) {
            workBuffer.sendBuffers.resize(sendCount);
        }
        for (auto & sendBuffer : workBuffer.sendBuffers) {
            if (sendBuffer.size() < bufferSize) {
                sendBuffer.resize(bufferSize, 0.0);
            }
        }
    }
}

void AudioEngine::ensureEffectWetBuffers(size_t effectCount, uint32_t bufferSize)
{
    if (m_effectWetBuffers.size() != effectCount) {
        m_effectWetBuffers.resize(effectCount);
    }

    for (auto & buffer : m_effectWetBuffers) {
        if (buffer.size() < bufferSize) {
            buffer.resize(bufferSize, 0.0);
        }
    }
}

void AudioEngine::ensureEffectActiveFlags(size_t effectCount)
{
    if (m_effectActiveFlags.size() != effectCount) {
        m_effectActiveFlags.assign(effectCount, 0);
    }
}

void AudioEngine::ensureDeviceActiveFlags(size_t deviceCount)
{
    if (m_deviceActiveFlags.size() != deviceCount) {
        m_deviceActiveFlags.assign(deviceCount, 0);
    }
}

} // namespace noteahead
