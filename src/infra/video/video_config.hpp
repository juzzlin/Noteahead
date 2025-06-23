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

#ifndef VIDEO_CONFIG_HPP
#define VIDEO_CONFIG_HPP

#include <chrono>
#include <string>

#include <QDateTime>
#include <QImage>

namespace noteahead {

struct VideoConfig
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
    double imageOpacity = 0.5;
    double imageZoomSpeed = 0;
    double imageRotationSpeed = 0;
    QImage image;

    std::optional<std::string> flashTrackName;
    std::optional<std::string> flashColumnName;

    double trackOpacity = 0.25;

    std::string logoPath;
    int logoX = 16;
    int logoY = 16;
    QImage logo;
    double logoFadeFactor = 1.0;

    std::string outputDir = "Frames-" + std::to_string(QDateTime::currentSecsSinceEpoch());

    size_t startPosition = 0;
    std::chrono::milliseconds leadInTime;
    std::chrono::milliseconds leadOutTime;
    std::optional<std::chrono::milliseconds> length;

    std::string scrollingText;
    double scrollingTextOpacity = 0.5;

    enum class Type
    {
        Default,
        Bars
    };

    Type type = Type::Default;
};

} // namespace noteahead

#endif // VIDEO_CONFIG_HPP
