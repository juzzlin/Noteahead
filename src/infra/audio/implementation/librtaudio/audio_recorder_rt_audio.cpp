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

    if (!inputBuffer || !self->m_sndFile) {
        return 0;
    }

    const auto in = static_cast<int32_t *>(inputBuffer);
    sf_writef_int(self->m_sndFile, in, nFrames);

    return 0;
}

AudioRecorderRtAudio::AudioRecorderRtAudio() = default;

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

void AudioRecorderRtAudio::start(const std::string & fileName, uint32_t bufferSize)
{
    if (!m_running) {

        try {
            if (m_rtAudio.getDeviceCount() < 1) {
                throw std::runtime_error("No audio devices found!");
            }

            const auto deviceId = m_rtAudio.getDefaultInputDevice();
            const auto deviceInfo = m_rtAudio.getDeviceInfo(deviceId);
            const uint32_t sampleRate = deviceInfo.preferredSampleRate ? deviceInfo.preferredSampleRate : 48000;
            const uint32_t channelCount = std::min(deviceInfo.inputChannels, 2u);

            juzzlin::L(TAG).info() << "Recording from device: " << deviceInfo.name << ", " << sampleRate << " Hz, " << channelCount << " channels (24-bit WAV)";
            juzzlin::L(TAG).info() << "Buffer size: " << bufferSize;

            initializeSoundFile(fileName, sampleRate, channelCount);
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
            stop();
        }

        if (m_sndFile) {
            sf_close(m_sndFile);
            m_sndFile = nullptr;
        }

        m_running = false;
        juzzlin::L(TAG).info() << "Recording stopped";
    }
}

bool AudioRecorderRtAudio::isRunning() const
{
    return m_running;
}

} // namespace noteahead
