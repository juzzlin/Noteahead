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

#include "../domain/event.hpp"
#include "../domain/song.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace noteahead {

class Instrument;
class MixerService;

class VideoGenerator
{
public:
    using MixerServiceS = std::shared_ptr<MixerService>;
    VideoGenerator(MixerServiceS mixerService);

    struct Config
    {
        int width = 1920;
        int height = 1080;

        size_t fps = 60;

        std::string song;
        std::string image;
        std::string outputDir;

        size_t startPosition = 0;
        std::chrono::milliseconds leadInTime;
        std::chrono::milliseconds leadOutTime;
        std::optional<std::chrono::milliseconds> length;

        std::string scrollingText;
    };

    using SongS = std::shared_ptr<Song>;
    void generateVideoFrames(SongS song, const Config & config);

private:
    void initialize(const Config & config);
    void generateAnimationFrames(SongS song, const Config & config);
    void generateVideoFrame(SongS song, const Config & config, QImage overlayImage, size_t frameIndex, double currentTimeMs);
    bool shouldEventPlay(const Event & event) const;

    MixerServiceS m_mixerService;

    SongS m_song;

    using EventMap = std::unordered_map<size_t, Song::EventList>;
    EventMap m_eventMap;

    using InstrumentS = std::shared_ptr<Instrument>;
    std::set<InstrumentS> m_allInstruments;

    struct AnimationFrame
    {
        struct Item
        {
            int x = 0;
            int y = 0;

            double r = 1.0;

            size_t track = 0;
            size_t column = 0;

            int midiNote = 0;
        };

        using ItemList = std::vector<Item>;
        using TrackIdToItemMap = std::map<size_t, ItemList>;
        TrackIdToItemMap trackToItemsMap;
    };

    using AnimationFrameS = std::shared_ptr<AnimationFrame>;
    using TickToAnimationFrameMap = std::map<size_t, AnimationFrameS>;
    TickToAnimationFrameMap m_animationFrames;

    Config m_config;

    size_t m_minTick = 0;
    size_t m_maxTick = 0;
};

} // namespace noteahead

#endif // VIDEO_GENERATOR_HPP
