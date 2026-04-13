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

#include "audio_player_jack.hpp"

#include "../../../application/service/jack_service.hpp"

namespace noteahead {

AudioPlayerJack::AudioPlayerJack(JackServiceS jackService)
  : m_jackService { std::move(jackService) }
{
}

AudioPlayerJack::~AudioPlayerJack() = default;

void AudioPlayerJack::start(const std::string & fileName, uint32_t)
{
    m_jackService->startPlayback(QString::fromStdString(fileName));
}

void AudioPlayerJack::stop()
{
    m_jackService->stopPlayback();
}

std::vector<AudioDevice> AudioPlayerJack::getOutputDevices()
{
    return {}; // Managed by JACK
}

void AudioPlayerJack::setOutputDevice(uint32_t)
{
}

void AudioPlayerJack::setPosition(double position)
{
    m_jackService->setPlaybackPosition(position);
}

double AudioPlayerJack::position() const
{
    return m_jackService->playbackPosition();
}

bool AudioPlayerJack::isFinished() const
{
    return m_jackService->isPlayingPlaybackFinished();
}

uint32_t AudioPlayerJack::sampleRate()
{
    return m_jackService->sampleRate();
}

} // namespace noteahead
