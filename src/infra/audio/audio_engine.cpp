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

#include "common/constants.hpp"
#include "common/denormal_protection.hpp"
#include "domain/effects/effect_rack.hpp"
#include "domain/dsp/reverb_effect.hpp"
#include "real_time_worker_pool.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

struct DeviceProcessContext
{
    std::vector<AudioEngine::DeviceS> * devices {};
    std::vector<AudioEngineWorkBuffer> * workBuffers {};
    std::vector<uint8_t> * deviceActiveFlags {};
    std::vector<double> * deviceSends {};
    size_t sendCount {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
    uint32_t bufferSize {};
};

struct EffectProcessContext
{
    std::vector<EffectRack::EffectS> * effects {};
    std::vector<std::vector<double>> * sendBusBuffers {};
    std::vector<std::vector<double>> * effectWetBuffers {};
    std::vector<uint8_t> * effectActiveFlags {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
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
    auto & device = deviceContext.devices->at(taskIndex);
    auto & workBuffer = deviceContext.workBuffers->at(workerIndex);

    std::fill(workBuffer.deviceBuffer.begin(), workBuffer.deviceBuffer.begin() + deviceContext.bufferSize, 0.0);
    if (!device->hasActiveAudio() && !deviceContext.deviceActiveFlags->at(taskIndex)) {
        return;
    }

    AudioContext audioContext { std::span(workBuffer.deviceBuffer.data(), deviceContext.bufferSize), deviceContext.frameCount, deviceContext.sampleRate };
    device->processAudio(audioContext);
    device->processInsertEffects(audioContext);
    const bool hasOutputSignal = bufferContainsSignal(workBuffer.deviceBuffer, deviceContext.bufferSize);
    deviceContext.deviceActiveFlags->at(taskIndex) = hasOutputSignal ? 1 : 0;

    for (uint32_t i = 0; i < deviceContext.bufferSize; i++) {
        const double sample = workBuffer.deviceBuffer[i];
        workBuffer.outputBuffer[i] += sample;

        for (size_t sendIndex = 0; sendIndex < deviceContext.sendCount; sendIndex++) {
            const double send = deviceContext.deviceSends->at(taskIndex * deviceContext.sendCount + sendIndex);
            workBuffer.sendBuffers[sendIndex][i] += sample * send;
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

    if (!bufferContainsSignal(sendBus, bufferSize) && !effectContext.effectActiveFlags->at(taskIndex)) {
        std::fill(wetBuffer.begin(), wetBuffer.begin() + bufferSize, 0.0);
        return;
    }

    effect->setSampleRate(effectContext.sampleRate);

    // Copy dry signal to wet buffer for in-place processing
    std::copy(sendBus.begin(), sendBus.begin() + bufferSize, wetBuffer.begin());

    AudioContext context_obj { std::span(wetBuffer.data(), bufferSize), effectContext.frameCount, effectContext.sampleRate };
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

    const uint32_t bufferSize = context.frameCount * 2;
    auto effects = m_sendEffectRack->effects();
    const size_t sendCount = effects.size();
    const size_t laneCount = m_workerPool->laneCount();

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
    if (!m_devices.empty()) {
        m_deviceSnapshot.reserve(m_devices.size());
        for (auto const & [index, device] : m_devices) {
            if (device) {
                m_deviceSnapshot.push_back(device);
            }
        }
    }

    if (!m_deviceSnapshot.empty()) {
        ensureDeviceActiveFlags(m_deviceSnapshot.size());

        m_deviceSendSnapshot.resize(m_deviceSnapshot.size() * sendCount);
        for (size_t deviceIndex = 0; deviceIndex < m_deviceSnapshot.size(); deviceIndex++) {
            for (size_t sendIndex = 0; sendIndex < sendCount; sendIndex++) {
                m_deviceSendSnapshot[deviceIndex * sendCount + sendIndex] = static_cast<double>(m_deviceSnapshot[deviceIndex]->reverbSend(sendIndex));
            }
        }

        for (auto & workBuffer : m_workBuffers) {
            std::fill(workBuffer.outputBuffer.begin(), workBuffer.outputBuffer.begin() + bufferSize, 0.0);
            for (auto & sendBuffer : workBuffer.sendBuffers) {
                std::fill(sendBuffer.begin(), sendBuffer.begin() + bufferSize, 0.0);
            }
        }

        DeviceProcessContext deviceContext {
            &m_deviceSnapshot,
            &m_workBuffers,
            &m_deviceActiveFlags,
            &m_deviceSendSnapshot,
            sendCount,
            context.frameCount,
            context.sampleRate,
            bufferSize
        };
        m_workerPool->run(m_deviceSnapshot.size(), &deviceContext, processDeviceTask);

        // Sum parallel results into the main output and send buses
        for (const auto & workBuffer : m_workBuffers) {
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

    if (std::ranges::any_of(effects, [](const auto & effect) { return effect != nullptr; })) {
        EffectProcessContext effectContext {
            &effects,
            &m_sendBusBuffers,
            &m_effectWetBuffers,
            &m_effectActiveFlags,
            context.frameCount,
            context.sampleRate
        };
        m_workerPool->run(sendCount, &effectContext, processEffectTask);

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

bool AudioEngine::isExclusive() const
{
    return m_isExclusive;
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
