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

#include <QDateTime>
#include <QImage>

class QPainter;

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

        std::string ffmpegPath = "ffmpeg";
        std::string audioCodec = "libfdk_aac";
        std::string videoCodec = "libx264";

        std::string audioPath;
        std::string songPath;
        std::string imagePath;
        QImage image;

        std::string logoPath;
        int logoX = 16;
        int logoY = 16;
        QImage logo;

        std::string outputDir = "Frames-" + std::to_string(QDateTime::currentSecsSinceEpoch());

        size_t startPosition = 0;
        std::chrono::milliseconds leadInTime;
        std::chrono::milliseconds leadOutTime;
        std::optional<std::chrono::milliseconds> length;

        std::string scrollingText;
    };

    using SongS = std::shared_ptr<Song>;
    void run(SongS song, const Config & config);

private:
    void initialize(const Config & config);

    void generateAnimationFrames(SongS song, const Config & config);
    void generateVideoFrame(SongS song, const Config & config, size_t frameIndex, double currentTimeMs);
    void generateVideoFrames(SongS song, const Config & config);

    void renderAnimationFrame(const Config & config, QPainter & painter, size_t frameIndex, double currentTimeMs);
    void renderVideo(const Config & config);

    void runCommand(const QStringList & args) const;

    bool shouldEventPlay(const Event & event) const;

    MixerServiceS m_mixerService;

    SongS m_song;

    using EventMap = std::unordered_map<size_t, Song::EventList>;
    EventMap m_eventMap;

    using InstrumentS = std::shared_ptr<Instrument>;
    std::set<InstrumentS> m_allInstruments;

    struct AnimationFrame
    {
        struct Particle
        {
            double x = 0;
            double y = 0;
            double vX = 0;
            double vY = 0;
            double aX = 0;
            double aY = 0;
            double r = 1.0;
            double a = 0.99;

            int midiNote = 0;
        };

        using ParticleList = std::vector<Particle>;
        ParticleList particles;
    };

    void integrate(AnimationFrame & animationFrame, double dt, double floor);

    using AnimationFrameS = std::shared_ptr<AnimationFrame>;
    using TickToAnimationFrameMap = std::map<size_t, AnimationFrameS>;
    TickToAnimationFrameMap m_animationFrames;

    Config m_config;

    size_t m_minTick = 0;
    size_t m_maxTick = 0;
};

} // namespace noteahead

#endif // VIDEO_GENERATOR_HPP
