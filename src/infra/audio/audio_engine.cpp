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
#include "../../domain/dsp/reverb_effect.hpp"

#include <algorithm>

namespace noteahead {

namespace {

struct DeviceProcessContext
{
    std::vector<AudioEngine::DeviceS> * devices {};
    std::vector<AudioEngineWorkBuffer> * workBuffers {};
    std::vector<float> * deviceSends {};
    size_t sendCount {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
    uint32_t bufferSize {};
};

struct EffectProcessContext
{
    std::vector<EffectRack::EffectS> * effects {};
    std::vector<std::vector<float>> * sendBusBuffers {};
    std::vector<std::vector<float>> * effectWetBuffers {};
    uint32_t frameCount {};
    uint32_t sampleRate {};
};

void processDeviceTask(void * context, size_t taskIndex, size_t workerIndex)
{
    auto & deviceContext = *static_cast<DeviceProcessContext *>(context);
    auto & device = deviceContext.devices->at(taskIndex);
    auto & workBuffer = deviceContext.workBuffers->at(workerIndex);

    std::fill(workBuffer.deviceBuffer.begin(), workBuffer.deviceBuffer.begin() + deviceContext.bufferSize, 0.0f);
    device->processAudio(workBuffer.deviceBuffer.data(), deviceContext.frameCount, deviceContext.sampleRate);

    for (uint32_t i = 0; i < deviceContext.bufferSize; ++i) {
        const float sample = workBuffer.deviceBuffer[i];
        workBuffer.outputBuffer[i] += sample;

        for (size_t sendIndex = 0; sendIndex < deviceContext.sendCount; ++sendIndex) {
            const float send = deviceContext.deviceSends->at(taskIndex * deviceContext.sendCount + sendIndex);
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

    effect->setSampleRate(effectContext.sampleRate);

    for (uint32_t i = 0; i < effectContext.frameCount; ++i) {
        const size_t leftIndex = i * 2;
        const size_t rightIndex = leftIndex + 1;
        const float dryL = sendBus[leftIndex];
        const float dryR = sendBus[rightIndex];

        float wetL = dryL;
        float wetR = dryR;
        effect->process(wetL, wetR);

        wetBuffer[leftIndex] = wetL - dryL;
        wetBuffer[rightIndex] = wetR - dryR;
    }
}

} // namespace

AudioEngine::AudioEngine()
{
    // Initialize with 4 independent reverbs
    for (int i = 0; i < 4; ++i) {
        m_effectRack.addEffect(std::make_shared<ReverbEffect>());
    }
}

AudioEngine::~AudioEngine() = default;

void AudioEngine::addDevice(DeviceS device)
{
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
    if (const auto it = m_devices.find(name); it != m_devices.end()) {
        return it->second;
    }
    return nullptr;
}

AudioEngine::DeviceNames AudioEngine::deviceNames() const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    DeviceNames names;
    for (auto const & [name, device] : m_devices) {
        names.push_back(name);
    }
    return names;
}

void AudioEngine::setBpm(float bpm)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & [name, device] : m_devices) {
        device->setBpm(bpm);
    }
}

void AudioEngine::process(float * output, uint32_t frameCount, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    const uint32_t bufferSize = frameCount * 2;
    auto effects = m_effectRack.effects();
    const size_t sendCount = effects.size();
    const size_t laneCount = m_workerPool.laneCount();

    ensureWorkBuffers(laneCount, sendCount, bufferSize);
    ensureEffectWetBuffers(sendCount, bufferSize);

    if (m_sendBusBuffers.size() != sendCount) {
        m_sendBusBuffers.resize(sendCount);
    }

    for (auto & bus : m_sendBusBuffers) {
        if (bus.size() < bufferSize) {
            bus.resize(bufferSize, 0.0f);
        } else {
            std::fill(bus.begin(), bus.begin() + bufferSize, 0.0f);
        }
    }

    m_deviceSnapshot.clear();
    m_deviceSnapshot.reserve(m_devices.size());
    for (auto const & [name, device] : m_devices) {
        m_deviceSnapshot.push_back(device);
    }

    m_deviceSendSnapshot.resize(m_deviceSnapshot.size() * sendCount);
    for (size_t deviceIndex = 0; deviceIndex < m_deviceSnapshot.size(); ++deviceIndex) {
        for (size_t sendIndex = 0; sendIndex < sendCount; ++sendIndex) {
            m_deviceSendSnapshot[deviceIndex * sendCount + sendIndex] = m_deviceSnapshot[deviceIndex]->reverbSend(sendIndex);
        }
    }

    for (auto & workBuffer : m_workBuffers) {
        std::fill(workBuffer.outputBuffer.begin(), workBuffer.outputBuffer.begin() + bufferSize, 0.0f);
        for (auto & sendBuffer : workBuffer.sendBuffers) {
            std::fill(sendBuffer.begin(), sendBuffer.begin() + bufferSize, 0.0f);
        }
    }

    DeviceProcessContext deviceContext {
        &m_deviceSnapshot,
        &m_workBuffers,
        &m_deviceSendSnapshot,
        sendCount,
        frameCount,
        sampleRate,
        bufferSize
    };
    m_workerPool.run(m_deviceSnapshot.size(), &deviceContext, processDeviceTask);

    for (const auto & workBuffer : m_workBuffers) {
        for (uint32_t i = 0; i < bufferSize; ++i) {
            output[i] += workBuffer.outputBuffer[i];
        }
        for (size_t sendIndex = 0; sendIndex < sendCount; ++sendIndex) {
            auto & sendBus = m_sendBusBuffers[sendIndex];
            const auto & laneSendBus = workBuffer.sendBuffers[sendIndex];
            for (uint32_t i = 0; i < bufferSize; ++i) {
                sendBus[i] += laneSendBus[i];
            }
        }
    }

    EffectProcessContext effectContext {
        &effects,
        &m_sendBusBuffers,
        &m_effectWetBuffers,
        frameCount,
        sampleRate
    };
    m_workerPool.run(sendCount, &effectContext, processEffectTask);

    for (const auto & wetBuffer : m_effectWetBuffers) {
        for (uint32_t i = 0; i < bufferSize; ++i) {
            output[i] += wetBuffer[i];
        }
    }
}

void AudioEngine::reset()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    for (auto const & [name, device] : m_devices) {
        device->resetAudio();
    }
    m_effectRack.reset();
}

void AudioEngine::setIsExclusive(bool exclusive)
{
    m_isExclusive = exclusive;
}

bool AudioEngine::isExclusive() const
{
    return m_isExclusive;
}

EffectRack & AudioEngine::effectRack()
{
    return m_effectRack;
}

void AudioEngine::ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize)
{
    if (m_workBuffers.size() != laneCount) {
        m_workBuffers.resize(laneCount);
    }

    for (auto & workBuffer : m_workBuffers) {
        if (workBuffer.deviceBuffer.size() < bufferSize) {
            workBuffer.deviceBuffer.resize(bufferSize, 0.0f);
        }
        if (workBuffer.outputBuffer.size() < bufferSize) {
            workBuffer.outputBuffer.resize(bufferSize, 0.0f);
        }
        if (workBuffer.sendBuffers.size() != sendCount) {
            workBuffer.sendBuffers.resize(sendCount);
        }
        for (auto & sendBuffer : workBuffer.sendBuffers) {
            if (sendBuffer.size() < bufferSize) {
                sendBuffer.resize(bufferSize, 0.0f);
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
            buffer.resize(bufferSize, 0.0f);
        }
    }
}

} // namespace noteahead
