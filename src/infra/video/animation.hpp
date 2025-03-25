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

#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "../../domain/song.hpp"
#include "video_config.hpp"

#include <memory>
#include <unordered_map>

class QPainter;

namespace noteahead {

class MixerService;
class Song;

class Animation
{
public:
    using SongS = std::shared_ptr<Song>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    Animation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick);
    virtual ~Animation();

    using EventMap = std::unordered_map<size_t, Song::EventList>;
    virtual void generateAnimationFrames(const EventMap & events) = 0;

    virtual void renderAnimationFrame(QPainter & painter, size_t frameIndex, double currentTimeMs) = 0;

protected:
    bool shouldEventPlay(const Event & event) const;

    size_t minTick() const;
    size_t maxTick() const;

    SongS song() const;
    VideoConfig config() const;
    MixerServiceS mixerService() const;

private:
    SongS m_song;
    VideoConfig m_config;
    MixerServiceS m_mixerService;

    size_t m_minTick = 0;
    size_t m_maxTick = 0;
};

} // namespace noteahead

#endif // ANIMATION_HPP
