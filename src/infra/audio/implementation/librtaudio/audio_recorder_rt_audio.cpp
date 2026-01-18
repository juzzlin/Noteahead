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

#include <chrono>
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
    const size_t totalSamples = nFrames * channels;

    if (!self->m_ringBuffer.push(in, totalSamples)) {
        std::cerr << "RingBuffer Overflow!" << std::endl;
    }

    return 0;
}

AudioRecorderRtAudio::AudioRecorderRtAudio()
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

void AudioRecorderRtAudio::initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize)
{
    RtAudio::StreamParameters iParams;
    iParams.deviceId = deviceId;
    iParams.nChannels = channelCount;
    uint32_t bufferFrames = bufferSize;
    m_rtAudio.openStream(nullptr, &iParams, RTAUDIO_SINT32,
                         sampleRate, &bufferFrames,
                         &AudioRecorderRtAudio::recordCallback, this);
    m_rtAudio.startStream();
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
    if (!m_running) {

        try {
            if (m_rtAudio.getDeviceCount() < 1) {
                throw std::runtime_error("No audio devices found!");
            }

            const auto deviceId = m_inputDeviceId.load();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            const uint32_t sampleRate = deviceInfo.preferredSampleRate ? deviceInfo.preferredSampleRate : 48000;
            const uint32_t channelCount = std::min(deviceInfo.inputChannels, 2u);

            juzzlin::L(TAG).info() << "Recording from device: " << deviceInfo.name << ", " << sampleRate << " Hz, " << channelCount << " channels (24-bit WAV)";
            juzzlin::L(TAG).info() << "Buffer size: " << bufferSize;

            initializeSoundFile(fileName, sampleRate, channelCount);

            // 2 seconds buffer
            m_ringBuffer.resize(sampleRate * channelCount * 2);

            m_stopThread = false;
            m_diskWriteThread = std::thread(&AudioRecorderRtAudio::diskWriteLoop, this);

            initializeSoundStream(deviceId, channelCount, sampleRate, bufferSize);

            m_running = true;
        } catch (std::exception & e) {
            juzzlin::L(TAG).error() << e.what();
            stop();
        }
    }
}

void AudioRecorderRtAudio::stop()
{
    if (m_running) {
        try {
            if (m_rtAudio.isStreamRunning()) {
                m_rtAudio.stopStream();
            }
            if (m_rtAudio.isStreamOpen()) {
                m_rtAudio.closeStream();
            }
        } catch (std::exception & e) {
            juzzlin::L(TAG).error() << e.what();
            // Avoid infinite recursion if stop() fails
            m_running = false;
        }

        m_stopThread = true;
        if (m_diskWriteThread.joinable()) {
            m_diskWriteThread.join();
        }

        if (m_sndFile) {
            sf_close(m_sndFile);
            m_sndFile = nullptr;
        }

        m_running = false;
        juzzlin::L(TAG).info() << "Recording stopped";
    }
}

} // namespace noteahead
