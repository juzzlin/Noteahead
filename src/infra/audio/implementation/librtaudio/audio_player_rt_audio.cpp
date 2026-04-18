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

#include <algorithm>
#include <chrono>
#include <iostream>

namespace noteahead {

static const auto TAG = "AudioPlayerRtAudio";

int AudioPlayerRtAudio::playCallback(void * outputBuffer, void *,
                                     uint32_t nFrames,
                                     double, RtAudioStreamStatus status,
                                     void * userData)
{
    const auto self = static_cast<AudioPlayerRtAudio *>(userData);

    if (status) {
        std::cerr << "Stream under/overflow detected!" << std::endl;
    }

    if (!outputBuffer) {
        return 0;
    }

    const auto out = static_cast<int32_t *>(outputBuffer);
    const int channels = self->m_sfInfo.channels;
    const size_t totalSamples = nFrames * channels;

    const size_t read = self->m_ringBuffer.pop(out, totalSamples);
    if (read < totalSamples) {
        // Zero out the rest of the buffer
        std::fill_n(out + read, totalSamples - read, 0);
    }

    if (read > 0) {
        const int channels = self->m_sfInfo.channels;
        if (channels > 0) {
            self->m_playedFrames += read / channels;
        }
    }

    // Update playback position
    if (self->m_sfInfo.frames > 0) {
        self->m_playbackPosition = static_cast<double>(self->m_playedFrames.load()) / self->m_sfInfo.frames;
        if (self->m_playedFrames >= static_cast<uint64_t>(self->m_sfInfo.frames)) {
            self->m_isFinished = true;
        }
    }

    return 0;
}

AudioPlayerRtAudio::AudioPlayerRtAudio(RtAudio::Api api)
  : m_rtAudio { api }
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
    for (unsigned int i = 0; i < deviceCount; ++i) {
        const auto info = m_rtAudio.getDeviceInfo(i);
        if (info.outputChannels > 0) {
            devices.push_back({ i, info.name });
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

void AudioPlayerRtAudio::initializeSoundFile(const std::string & fileName)
{
    m_sfInfo = {};
    m_sndFile = sf_open(fileName.c_str(), SFM_READ, &m_sfInfo);
    if (!m_sndFile) {
        throw std::runtime_error("Error opening file for reading: " + fileName);
    }
}

void AudioPlayerRtAudio::initializeSoundStream(uint32_t deviceId, uint32_t channelCount, uint32_t sampleRate, uint32_t bufferSize)
{
    RtAudio::StreamParameters oParams;
    oParams.deviceId = deviceId;
    oParams.nChannels = channelCount;
    uint32_t bufferFrames = bufferSize;
    m_rtAudio.openStream(&oParams, nullptr, RTAUDIO_SINT32,
                         sampleRate, &bufferFrames,
                         &AudioPlayerRtAudio::playCallback, this);
    m_rtAudio.startStream();
}

void AudioPlayerRtAudio::diskReadLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopThread) {
        const size_t available = m_ringBuffer.writeAvailable();
        if (available > 0 && m_sndFile) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const int channels = m_sfInfo.channels;
            if (channels > 0) {
                const sf_count_t read = sf_readf_int(m_sndFile, tempBuffer.data(), toRead / channels);
                if (read > 0) {
                    m_ringBuffer.push(tempBuffer.data(), read * channels);
                } else {
                    // EOF or error
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void AudioPlayerRtAudio::start(const std::string & fileName, uint32_t bufferSize)
{
    if (!m_running) {
        try {
            m_isFinished = false;
            initializeSoundFile(fileName);

            uint32_t sampleRate = m_sfInfo.samplerate;
            uint32_t channelCount = m_sfInfo.channels;
            uint32_t deviceId = 0;
            std::string deviceName = "JACK System";
            uint32_t actualBufferSize = bufferSize;

            if (m_rtAudio.getCurrentApi() == RtAudio::UNIX_JACK) {
                juzzlin::L(TAG).info() << "JACK backend detected, skipping device probe.";
                deviceId = m_rtAudio.getDefaultOutputDevice();
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

            juzzlin::L(TAG).info() << "Playing to device: " << deviceName << ", " << sampleRate << " Hz, " << channelCount << " channels";

            // 500ms buffer
            m_ringBuffer.resize(sampleRate * channelCount / 2);

            // Respect existing position if set
            if (m_playbackPosition > 0.0 && m_playbackPosition < 1.0) {
                const sf_count_t targetFrame = static_cast<sf_count_t>(m_playbackPosition * m_sfInfo.frames);
                sf_seek(m_sndFile, targetFrame, SEEK_SET);
                m_playedFrames = targetFrame;
            } else {
                m_playedFrames = 0;
                m_playbackPosition = 0.0;
            }

            m_stopThread = false;
            m_diskReadThread = std::thread(&AudioPlayerRtAudio::diskReadLoop, this);

            initializeSoundStream(deviceId, channelCount, sampleRate, actualBufferSize);

            m_running = true;
        } catch (...) {
            stop();
            throw;
        }
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

    m_stopThread = true;
    if (m_diskReadThread.joinable()) {
        m_diskReadThread.join();
    }

    if (m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
    }

    if (wasRunning) {
        juzzlin::L(TAG).info() << "Playback stopped";
    }
}

void AudioPlayerRtAudio::setPosition(double position)
{
    if (m_sndFile && m_sfInfo.frames > 0) {
        const sf_count_t targetFrame = static_cast<sf_count_t>(position * m_sfInfo.frames);
        sf_seek(m_sndFile, targetFrame, SEEK_SET);
        m_ringBuffer.clear();
        m_playedFrames = targetFrame;
        m_playbackPosition = position;
        m_isFinished = targetFrame >= m_sfInfo.frames;
    }
}

double AudioPlayerRtAudio::position() const
{
    return m_playbackPosition.load();
}

bool AudioPlayerRtAudio::isFinished() const
{
    return m_isFinished.load();
}

} // namespace noteahead
