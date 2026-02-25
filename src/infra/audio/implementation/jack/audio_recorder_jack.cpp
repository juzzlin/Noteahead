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

#include "audio_recorder_jack.hpp"
#include "../../../../application/service/jack_service.hpp"

#include <QString>

namespace noteahead {

AudioRecorderJack::AudioRecorderJack(JackServiceS jackService)
  : m_jackService { std::move(jackService) }
{
}

AudioRecorderJack::~AudioRecorderJack() = default;

void AudioRecorderJack::start(const std::string & fileName, uint32_t /*bufferSize*/)
{
    if (m_jackService) {
        m_jackService->startRecording(QString::fromStdString(fileName));
    }
}

void AudioRecorderJack::stop()
{
    if (m_jackService) {
        m_jackService->stopRecording();
    }
}

std::vector<AudioDevice> AudioRecorderJack::getInputDevices()
{
    // JACK ports are managed differently, but we could return something here if needed.
    // For now, return a placeholder or empty.
    return { { 0, "JACK System" } };
}

void AudioRecorderJack::setInputDevice(uint32_t /*deviceId*/)
{
    // Implementation for setting input device if applicable for JACK
}

uint32_t AudioRecorderJack::sampleRate()
{
    return m_jackService ? m_jackService->sampleRate() : 48000;
}

} // namespace noteahead
