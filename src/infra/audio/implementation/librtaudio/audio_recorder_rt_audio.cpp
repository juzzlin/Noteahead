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
    const int channels = self->m_sfInfo.channels;
    if (channels <= 0) {
        return 0;
    }

    const size_t totalSamples = nFrames * channels;

    // We no longer call m_audioEngine->process() here because it's already called by the player.
    // Calling it twice per period causes everything in the engine (like sampler voices) to advance twice as fast.
    // Consistent with JackService, we only record hardware input.

    if (!self->m_ringBuffer.push(in, totalSamples)) {
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

void AudioRecorderRtAudio::initializeSoundFile(const std::string & fileName, uint32_t sampleRate, uint32_t channelCount)
{
    // Open WAV file: 24-bit PCM
    m_sfInfo = {};
    m_sfInfo.samplerate = static_cast<int>(sampleRate);
    m_sfInfo.channels = static_cast<int>(channelCount);
    m_sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    m_sndFile = sf_open(fileName.c_str(), SFM_WRITE, &m_sfInfo);
    if (!m_sndFile) {
        throw std::runtime_error("Error opening file: " + fileName);
    }
}

uint32_t AudioRecorderRtAudio::initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize)
{
    RtAudio::StreamParameters iParams;
    iParams.deviceId = deviceId;
    iParams.nChannels = channelCount;
    uint32_t bufferFrames = bufferSize;
    m_rtAudio.openStream(nullptr, &iParams, RTAUDIO_SINT32,
                         sampleRate, &bufferFrames,
                         &AudioRecorderRtAudio::recordCallback, this);
    m_rtAudio.startStream();
    return m_rtAudio.getStreamSampleRate();
}

void AudioRecorderRtAudio::diskWriteLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopThread || m_ringBuffer.readAvailable() > 0) {
        const size_t available = m_ringBuffer.readAvailable();
        if (available > 0) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const size_t read = m_ringBuffer.pop(tempBuffer.data(), toRead);
            if (read > 0 && m_sndFile) {
                const int channels = m_sfInfo.channels;
                if (channels > 0) {
                    sf_writef_int(m_sndFile, tempBuffer.data(), read / channels);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
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

        const uint32_t actualSampleRate = initializeSoundStream(deviceId, channelCount, sampleRate, actualBufferSize);

        juzzlin::L(TAG).info() << "Recording from device: " << deviceName << ", " << actualSampleRate << " Hz, " << channelCount << " channels (24-bit WAV)";
        juzzlin::L(TAG).info() << "Buffer size: " << (actualBufferSize == 0 ? "Default" : std::to_string(actualBufferSize));

        initializeSoundFile(fileName, actualSampleRate, channelCount);

        // 2 seconds buffer
        m_ringBuffer.resize(actualSampleRate * channelCount * 2);

        m_stopThread = false;
        m_diskWriteThread = std::thread(&AudioRecorderRtAudio::diskWriteLoop, this);

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

    m_stopThread = true;
    if (m_diskWriteThread.joinable()) {
        m_diskWriteThread.join();
    }

    if (m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
    }

    if (wasRunning) {
        juzzlin::L(TAG).info() << "Recording stopped";
    }
}

} // namespace noteahead
