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

#include "audio_player_rt_audio.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../audio_engine.hpp"

#include <algorithm>
#include <iostream>

namespace noteahead {

static const auto TAG = "AudioPlayerRtAudio";

int AudioPlayerRtAudio::playCallback(void * outputBuffer, void *,
                                     uint32_t frameCount,
                                     double, RtAudioStreamStatus status,
                                     void * userData)
{
    const auto self = static_cast<AudioPlayerRtAudio *>(userData);

    if (status) {
        juzzlin::L(TAG).error() << "Stream under/overflow detected!";
    }

    if (!outputBuffer) {
        return 0;
    }

    const auto out = static_cast<int32_t *>(outputBuffer);
    const size_t channels = self->m_streamer.channels() > 0 ? static_cast<size_t>(self->m_streamer.channels()) : 2; // Default to 2 if no file
    const size_t totalSamples = frameCount * channels;

    if (const size_t read = self->m_streamer.pop(out, totalSamples); read < totalSamples) {
        // Zero out the rest of the buffer
        std::fill_n(out + read, totalSamples - read, 0);
    }

    if (self->m_audioEngine) {
        std::vector<float> interleaved(frameCount * 2, 0.0f);
        self->m_audioEngine->process(interleaved.data(), frameCount, self->m_rtAudio.getStreamSampleRate());
        for (uint32_t frame = 0; frame < frameCount; frame++) {
            // Mix with existing buffer (converted to float)
            // assuming int32_t full scale
            for (size_t channel = 0; channel < channels; channel++) {
                const auto outIndex = frame * channels + channel;
                const auto maxVal = 2'147'483'647.0f;
                // Mix in engine data (interleaved is always 2 channels)
                const auto currentVal = static_cast<float>(out[outIndex]) / maxVal + interleaved[frame * 2 + (channel % 2)];
                out[outIndex] = static_cast<int32_t>(std::clamp(currentVal, -1.0f, 1.0f) * maxVal);
            }
        }
    }

    return 0;
}

AudioPlayerRtAudio::AudioPlayerRtAudio(AudioEngineS audioEngine, RtAudio::Api api)
  : AudioPlayer { std::move(audioEngine) }
  , m_rtAudio { api }
{
    if (m_rtAudio.getDeviceCount() > 0) {
        m_outputDeviceId = m_rtAudio.getDefaultOutputDevice();
    }
}

void AudioPlayerRtAudio::setOutputDevice(uint32_t deviceId)
{
    m_outputDeviceId = deviceId;
}

std::vector<AudioDevice> AudioPlayerRtAudio::getOutputDevices()
{
    std::vector<AudioDevice> devices;
    const unsigned int deviceCount = m_rtAudio.getDeviceCount();
    for (unsigned int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
        const auto info = m_rtAudio.getDeviceInfo(deviceIndex);
        if (info.outputChannels > 0) {
            devices.push_back({ deviceIndex, info.name });
        }
    }
    return devices;
}

uint32_t AudioPlayerRtAudio::sampleRate()
{
    if (m_rtAudio.isStreamOpen()) {
        return m_rtAudio.getStreamSampleRate();
    }
    return 48000;
}

AudioPlayerRtAudio::~AudioPlayerRtAudio()
{
    AudioPlayerRtAudio::stop();
}

uint32_t AudioPlayerRtAudio::initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize)
{
    RtAudio::StreamParameters streamParameters;
    streamParameters.deviceId = deviceId;
    streamParameters.nChannels = channelCount;
    streamParameters.firstChannel = 0;

    RtAudio::StreamOptions streamOptions;
    streamOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
    // ALSA defaults this to 4 or 8 in many setups, which multiplies latency.
    // 2 is the standard for double-buffered low-latency audio.
    streamOptions.numberOfBuffers = 2;
    // Set a priority name for the thread (helpful for debugging in 'top' or 'htop')
    streamOptions.streamName = TAG;

    uint32_t bufferFrames = bufferSize;
    m_rtAudio.openStream(&streamParameters, nullptr, RTAUDIO_SINT32,
                         sampleRate, &bufferFrames,
                         &AudioPlayerRtAudio::playCallback, this, &streamOptions);
    m_rtAudio.startStream();

    return m_rtAudio.getStreamSampleRate();
}

void AudioPlayerRtAudio::start(const std::string & fileName, uint32_t bufferSize)
{
    if (m_running) {
        stop();
    }

    try {
        uint32_t sampleRate = 44100;
        uint32_t channelCount = 2;

        if (!fileName.empty()) {
            m_streamer.start(fileName, 0); // Temporary size, will resize below
            sampleRate = static_cast<uint32_t>(m_streamer.sampleRate());
            channelCount = static_cast<uint32_t>(m_streamer.channels());
        }

        uint32_t deviceId = 0;
        std::string deviceName = "JACK System";
        uint32_t actualBufferSize = bufferSize;

        if (m_rtAudio.getCurrentApi() == RtAudio::UNIX_JACK) {
            juzzlin::L(TAG).info() << "JACK backend detected, skipping device probe.";
            deviceId = m_rtAudio.getDefaultInputDevice();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            deviceName = deviceInfo.name;
            actualBufferSize = 0; // Let JACK decide
        } else {
            if (m_rtAudio.getDeviceCount() < 1) {
                throw std::runtime_error("No audio devices found!");
            }

            deviceId = m_outputDeviceId.load();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            deviceName = deviceInfo.name;
        }

        const uint32_t actualSampleRate = initializeSoundStream(deviceId, channelCount, sampleRate, actualBufferSize);

        juzzlin::L(TAG).info() << "Playing to device: " << deviceName << ", " << actualSampleRate << " Hz, " << channelCount << " channels";

        // 500ms buffer
        if (!fileName.empty()) {
            m_streamer.start(fileName, actualSampleRate * channelCount / 2, position());
        }

        m_running = true;
    } catch (...) {
        stop();
        throw;
    }
}

void AudioPlayerRtAudio::stop()
{
    const bool wasRunning = m_running;
    m_running = false;

    try {
        if (m_rtAudio.isStreamRunning()) {
            m_rtAudio.stopStream();
        }
        if (m_rtAudio.isStreamOpen()) {
            m_rtAudio.closeStream();
        }
    } catch (std::exception & e) {
        juzzlin::L(TAG).error() << e.what();
    }

    m_streamer.stop();

    if (wasRunning) {
        juzzlin::L(TAG).info() << "Playback stopped";
    }
}

void AudioPlayerRtAudio::setPosition(double position)
{
    m_streamer.setPosition(position);
}

double AudioPlayerRtAudio::position() const
{
    return m_streamer.position();
}

bool AudioPlayerRtAudio::isFinished() const
{
    return m_streamer.isFinished();
}

} // namespace noteahead
