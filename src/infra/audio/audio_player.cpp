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

#include "audio_player.hpp"

namespace noteahead {

AudioPlayer::AudioPlayer(AudioEngineS audioEngine)
  : m_audioEngine { std::move(audioEngine) }
{
}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::start(const std::string &, uint32_t)
{
}

void AudioPlayer::stop()
{
}

std::vector<AudioDevice> AudioPlayer::getOutputDevices()
{
    return {};
}

void AudioPlayer::setOutputDevice(uint32_t)
{
}

void AudioPlayer::setPosition(double)
{
}

double AudioPlayer::position() const
{
    return 0.0;
}

bool AudioPlayer::isFinished() const
{
    return false;
}

uint32_t AudioPlayer::sampleRate()
{
    return 48000;
}

} // namespace noteahead
