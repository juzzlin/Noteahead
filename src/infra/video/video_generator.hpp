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

#ifndef VIDEO_GENERATOR_HPP
#define VIDEO_GENERATOR_HPP

#include "../../domain/event.hpp"
#include "../../domain/song.hpp"
#include "video_config.hpp"

#include <memory>

#include <QDateTime>
#include <QImage>

class QPainter;

namespace noteahead {

class Animation;
class MixerService;

class VideoGenerator
{
public:
    using MixerServiceS = std::shared_ptr<MixerService>;
    VideoGenerator(MixerServiceS mixerService);
    ~VideoGenerator();

    using SongS = std::shared_ptr<Song>;
    void run(SongS song, const VideoConfig & config);

private:
    void initialize(const VideoConfig & config);

    void generateVideoFrame(SongS song, const VideoConfig & config, size_t frameIndex, double currentTimeMs);
    void generateVideoFrames(SongS song, const VideoConfig & config);
    void renderVideo(const VideoConfig & config);

    void runCommand(const QStringList & args) const;

    MixerServiceS m_mixerService;

    SongS m_song;

    using EventMap = std::unordered_map<size_t, Song::EventList>;
    EventMap m_eventMap;

    std::unique_ptr<Animation> m_animation;

    VideoConfig m_config;

    size_t m_minTick = 0;
    size_t m_maxTick = 0;
};

} // namespace noteahead

#endif // VIDEO_GENERATOR_HPP
