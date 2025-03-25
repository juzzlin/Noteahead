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

#include "animation.hpp"

#include "../../application/mixer_service.hpp"
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"

namespace noteahead {

Animation::Animation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick)
  : m_song { song }
  , m_config { config }
  , m_mixerService { mixerService }
  , m_minTick { minTick }
  , m_maxTick { maxTick }
{
}

bool Animation::shouldEventPlay(const Event & event) const
{
    if (auto && noteData = event.noteData(); noteData) {
        return m_mixerService->shouldColumnPlay(noteData->track(), noteData->column());
    }
    return true;
}

size_t Animation::maxTick() const
{
    return m_maxTick;
}

VideoConfig Animation::config() const
{
    return m_config;
}

Animation::SongS Animation::song() const
{
    return m_song;
}

size_t Animation::minTick() const
{
    return m_minTick;
}

Animation::MixerServiceS Animation::mixerService() const
{
    return m_mixerService;
}

Animation::~Animation() = default;

} // namespace noteahead
