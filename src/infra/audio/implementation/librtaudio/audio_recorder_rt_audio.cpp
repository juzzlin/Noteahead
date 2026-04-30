// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "audio_recorder_rt_audio.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../audio_engine.hpp"

#include <algorithm>

#include <iostream>

namespace noteahead {

static const auto TAG = "AudioRecorderRtAudio";

int AudioRecorderRtAudio::recordCallback(void *, void * inputBuffer,
                                         uint32_t nFrames,
                                         double, RtAudioStreamStatus status,
                                         void * userData)
{
    const auto self = static_cast<AudioRecorderRtAudio *>(userData);

    if (status) {
        std::cerr << "Stream under/overflow detected!" << std::endl;
    }

    if (!inputBuffer) {
        return 0;
    }

    const auto in = static_cast<int32_t *>(inputBuffer);
    const size_t totalSamples = nFrames * self->m_channels;

    if (!self->m_recorder.push(in, totalSamples)) {
        std::cerr << "RingBuffer Overflow!" << std::endl;
    }

    return 0;
}

AudioRecorderRtAudio::AudioRecorderRtAudio(AudioEngineS audioEngine, RtAudio::Api api)
  : AudioRecorder { std::move(audioEngine) }
  , m_rtAudio { api }
{
    if (m_rtAudio.getDeviceCount() > 0) {
        m_inputDeviceId = m_rtAudio.getDefaultInputDevice();
    }
}

void AudioRecorderRtAudio::setInputDevice(uint32_t deviceId)
{
    m_inputDeviceId = deviceId;
}

std::vector<AudioDevice> AudioRecorderRtAudio::getInputDevices()
{
    std::vector<AudioDevice> devices;
    const unsigned int deviceCount = m_rtAudio.getDeviceCount();
    for (unsigned int i = 0; i < deviceCount; ++i) {
        const auto info = m_rtAudio.getDeviceInfo(i);
        if (info.inputChannels > 0) {
            devices.push_back({ i, info.name });
        }
    }
    return devices;
}

uint32_t AudioRecorderRtAudio::sampleRate()
{
    if (m_rtAudio.isStreamOpen()) {
        return m_rtAudio.getStreamSampleRate();
    }
    return 48000;
}

AudioRecorderRtAudio::~AudioRecorderRtAudio()
{
    AudioRecorderRtAudio::stop();
}

uint32_t AudioRecorderRtAudio::initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize)
{
    RtAudio::StreamParameters streamParameters;
    streamParameters.deviceId = deviceId;
    streamParameters.nChannels = channelCount;
    streamParameters.firstChannel = 0;

    RtAudio::StreamOptions streamOptions;
    // Minimize latency for the input stream and request realtime scheduling
    streamOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
    // Set to 2 buffers to minimize the time between hardware capture and your callback
    streamOptions.numberOfBuffers = 2;
    streamOptions.streamName = "NoteaheadRecorder";

    try {
        uint32_t bufferFrames = bufferSize;
        m_rtAudio.openStream(nullptr, &streamParameters, RTAUDIO_SINT32,
                             sampleRate, &bufferFrames,
                             &AudioRecorderRtAudio::recordCallback, this, &streamOptions);
        m_rtAudio.startStream();
    } catch (RtAudioError & e) {
        // In recording, if 'hw:' is busy, this will catch the 'Device Busy' error
        e.printMessage();
        return 0;
    }

    return m_rtAudio.getStreamSampleRate();
}

void AudioRecorderRtAudio::start(const std::string & fileName, uint32_t bufferSize)
{
    if (m_running) {
        stop();
    }

    try {
        uint32_t sampleRate = 48000;
        uint32_t channelCount = 2;
        uint32_t deviceId = 0;
        std::string deviceName = "JACK System";
        uint32_t actualBufferSize = bufferSize;

        if (m_rtAudio.getCurrentApi() == RtAudio::UNIX_JACK) {
            juzzlin::L(TAG).info() << "JACK backend detected, skipping device probe.";
            deviceId = m_rtAudio.getDefaultInputDevice();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            sampleRate = deviceInfo.preferredSampleRate;
            deviceName = deviceInfo.name;
            actualBufferSize = 0; // Let JACK decide
        } else {
            if (m_rtAudio.getDeviceCount() < 1) {
                throw std::runtime_error("No audio devices found!");
            }

            deviceId = m_inputDeviceId.load();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            sampleRate = deviceInfo.preferredSampleRate ? deviceInfo.preferredSampleRate : 48000;
            channelCount = std::min(deviceInfo.inputChannels, 2u);
            deviceName = deviceInfo.name;
        }

        m_channels = channelCount;
        const uint32_t actualSampleRate = initializeSoundStream(deviceId, channelCount, sampleRate, actualBufferSize);

        juzzlin::L(TAG).info() << "Recording from device: " << deviceName << ", " << actualSampleRate << " Hz, " << channelCount << " channels (24-bit WAV)";
        juzzlin::L(TAG).info() << "Buffer size: " << (actualBufferSize == 0 ? "Default" : std::to_string(actualBufferSize));

        // 2 seconds buffer
        m_recorder.start(fileName, actualSampleRate, channelCount, actualSampleRate * channelCount * 2);

        m_running = true;
    } catch (...) {
        stop();
        throw;
    }
}

void AudioRecorderRtAudio::stop()
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

    m_recorder.stop();

    if (wasRunning) {
        juzzlin::L(TAG).info() << "Recording stopped";
    }
}

} // namespace noteahead
