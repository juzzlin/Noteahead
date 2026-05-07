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

namespace noteahead {

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
    if (m_deviceBuffer.size() < bufferSize) {
        m_deviceBuffer.resize(bufferSize, 0.0f);
    }

    const size_t sendCount = m_effectRack.effectCount();
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

    for (auto const & [name, device] : m_devices) {
        std::fill(m_deviceBuffer.begin(), m_deviceBuffer.begin() + bufferSize, 0.0f);
        device->processAudio(m_deviceBuffer.data(), frameCount, sampleRate);

        const float rSend0 = device->reverbSend(0);
        const float rSend1 = device->reverbSend(1);
        const float rSend2 = device->reverbSend(2);
        const float rSend3 = device->reverbSend(3);

        for (uint32_t i = 0; i < bufferSize; ++i) {
            const float sample = m_deviceBuffer.at(i);
            output[i] += sample;
            
            if (sendCount > 0) m_sendBusBuffers.at(0).at(i) += sample * rSend0;
            if (sendCount > 1) m_sendBusBuffers.at(1).at(i) += sample * rSend1;
            if (sendCount > 2) m_sendBusBuffers.at(2).at(i) += sample * rSend2;
            if (sendCount > 3) m_sendBusBuffers.at(3).at(i) += sample * rSend3;
        }
    }

    // Process Effect Rack
    for (size_t s = 0; s < sendCount; ++s) {
        m_effectRack.process(output, m_sendBusBuffers[s].data(), s, frameCount, sampleRate);
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

} // namespace noteahead
