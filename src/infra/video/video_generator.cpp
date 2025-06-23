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

#include "video_generator.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../common/utils.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "bars_animation.hpp"
#include "default_animation.hpp"

#include <algorithm>
#include <future>
#include <iomanip>
#include <ranges>
#include <semaphore>

#include <QDir>
#include <QPainter>
#include <QProcess>

namespace noteahead {

static const auto TAG = "VideoGenerator";

VideoGenerator::VideoGenerator(MixerServiceS mixerService)
  : m_mixerService { mixerService }
{
}

void VideoGenerator::initialize(const VideoConfig & config)
{
    ensureInputFilesExist(config);

    m_config = config;

    m_config.image = !m_config.imagePath.empty() ? QImage { QString::fromStdString(m_config.imagePath) } : QImage {};
    m_config.logo = !m_config.logoPath.empty() ? QImage { QString::fromStdString(m_config.logoPath) } : QImage {};

    m_eventMap.clear();
    for (auto && event : m_song->renderToEvents(std::make_shared<AutomationService>(), config.startPosition)) {
        m_eventMap[event->tick()].push_back(event);
    }
    const double tickDurationMs = 60'000 / (static_cast<double>(m_song->beatsPerMinute() * m_song->linesPerBeat() * m_song->ticksPerLine()));
    const auto [minTick, maxTick] = std::ranges::minmax(m_eventMap | std::views::keys);
    m_minTick = minTick;
    m_maxTick = maxTick + static_cast<size_t>(static_cast<double>(config.leadOutTime.count()) / tickDurationMs);
    juzzlin::L(TAG).info() << "Min tick: " << m_minTick;
    juzzlin::L(TAG).info() << "Max tick: " << m_maxTick;
    m_maxTick += static_cast<size_t>(static_cast<double>(config.leadOutTime.count()) / tickDurationMs);
    juzzlin::L(TAG).info() << "Max tick (+ lead-out time): " << m_maxTick;
}

void VideoGenerator::ensureInputFilesExist(const VideoConfig & config)
{
    Utils::ensureFileExists(config.audioPath);
    Utils::ensureFileExists(config.imagePath);
    Utils::ensureFileExists(config.logoPath);
    Utils::ensureFileExists(config.songPath);
}

void VideoGenerator::generateVideoFrames(SongS song, const VideoConfig & config)
{
    const auto fps = m_config.fps;
    const auto totalFrames = config.length.has_value()
      ? fps * static_cast<size_t>(config.length->count() + config.leadInTime.count() + config.leadOutTime.count()) / 1'000
      : fps * static_cast<size_t>(song->duration(config.startPosition).count() + config.leadInTime.count() + config.leadOutTime.count()) / 1'000;
    const double frameDurationMs = 1'000.0 / static_cast<double>(fps);

    juzzlin::L(TAG).info() << "Generating " << totalFrames << " video frames at " << fps << " FPS";

    const auto durationMs = static_cast<size_t>(frameDurationMs * static_cast<double>(totalFrames));
    const auto hours = durationMs / (1'000 * 60 * 60);
    const auto minutes = (durationMs / (1'000 * 60)) % 60;
    const auto seconds = (durationMs / 1'000) % 60;
    const auto millis = durationMs % 1'000;

    juzzlin::L(TAG).info() << "Video duration: "
                           << std::setw(2) << std::setfill('0') << hours << ":"
                           << std::setw(2) << std::setfill('0') << minutes << ":"
                           << std::setw(2) << std::setfill('0') << seconds << "."
                           << std::setw(3) << std::setfill('0') << millis;

    const auto outputDir = QString::fromStdString(config.outputDir);
    juzzlin::L(TAG).debug() << "Writing frames to " << std::quoted(outputDir.toStdString());
    if (QDir dir { outputDir }; !dir.exists()) {
        juzzlin::L(TAG).info() << "Creating " << std::quoted(outputDir.toStdString());
        dir.mkpath(outputDir);
    }

    // Limit concurrent threads using a dynamically allocated semaphore
    const auto maxThreads = std::max(1u, std::thread::hardware_concurrency()); // At least 1 thread
    const auto semaphore = std::make_unique<std::counting_semaphore<>>(maxThreads);
    std::vector<std::future<void>> futures;
    for (size_t frameIndex = 0; frameIndex < totalFrames; frameIndex++) {
        semaphore->acquire(); // Wait for a thread slot to be available
        futures.push_back(std::async(std::launch::async, [&, frameIndex]() {
            const double currentTimeMs = frameDurationMs * static_cast<double>(frameIndex);
            juzzlin::L(TAG).info() << "Frame " << frameIndex + 1 << "/" << totalFrames; // SimpleLogger is thread-safe
            generateVideoFrame(song, config, frameIndex, currentTimeMs);
            semaphore->release(); // Release the thread slot when done
        }));
    }

    // Ensure all tasks complete before finishing
    for (auto & future : futures) {
        future.get();
    }
}

void VideoGenerator::runCommand(const QStringList & args) const
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(args[0], args.mid(1));

    if (!process.waitForStarted()) {
        throw std::runtime_error("Failed to start process: " + args.at(0).toStdString());
    }

    while (process.waitForReadyRead()) {
        juzzlin::L(TAG).info() << process.readAll().toStdString();
    }

    process.waitForFinished();
    juzzlin::L(TAG).info() << "Process finished with exit code:" << process.exitCode();
}

void VideoGenerator::renderVideo(const VideoConfig & config)
{
    juzzlin::L(TAG).info() << "Rendering video..";

    // Run ffmpeg
    QStringList ffmpegArgs {
        QString::fromStdString(config.ffmpegPath),
        "-y",
        "-framerate", QString::number(config.fps),
        "-i", QString::fromStdString(config.outputDir) + "/frame_%05d.png",
        "-i", QString::fromStdString(config.audioPath),
        "-c:v", QString::fromStdString(config.videoCodec),
        "-r", QString::number(config.fps),
        "-pix_fmt", "yuv420p",
        "-c:a", QString::fromStdString(config.audioCodec),
        "-b:a", "320k",
        "-ar", "48000",
        QString::fromStdString(config.songPath) + ".mp4"
    };

    runCommand(ffmpegArgs);
}

void VideoGenerator::run(SongS song, const VideoConfig & config)
{
    if (m_song = song; !m_song) {
        return;
    }

    initialize(config);

    m_animation = config.type == VideoConfig::Type::Bars ? std::unique_ptr<Animation>(std::make_unique<BarsAnimation>(song, config, m_mixerService, m_minTick, m_maxTick))
                                                         : std::unique_ptr<Animation>(std::make_unique<DefaultAnimation>(song, config, m_mixerService, m_minTick, m_maxTick));
    m_animation->generateAnimationFrames(m_eventMap);

    generateVideoFrames(song, config);

    renderVideo(config);

    juzzlin::L(TAG).info() << "Done.";
}

void renderTrackBackgrounds(VideoGenerator::SongS song, const VideoConfig & config, QPainter & painter)
{
    painter.save();
    static const std::vector<QString> trackColors = {
        "#000000", "#ffffff"
    };
    const int trackHeight = config.height / static_cast<int>(song->trackCount());
    for (size_t trackIndex = 0; trackIndex < song->trackCount(); trackIndex++) {
        const QColor trackColor { trackColors.at(trackIndex % trackColors.size()) };
        QRect trackRect { 0, static_cast<int>(trackIndex) * trackHeight, static_cast<int>(config.width), static_cast<int>(trackHeight) };
        painter.setOpacity(config.trackOpacity);
        painter.fillRect(trackRect, trackColor);
    }
    painter.restore();
}

void renderImage(const VideoConfig & config, QPainter & painter, size_t frameIndex)
{
    painter.save();

    if (!config.image.isNull()) {

        // Fill the entire area with black first
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.drawRect(QRect(0, 0, config.width, config.height));

        const int imageHeight = config.height;
        const double aspectRatio = static_cast<double>(config.image.width()) / config.image.height();
        const int imageWidth = static_cast<int>(imageHeight * aspectRatio);

        const double zoomFactor = 1.0 + static_cast<double>(frameIndex) * config.imageZoomSpeed;
        const double zoomedWidth = imageWidth * zoomFactor;
        const double zoomedHeight = imageHeight * zoomFactor;
        const double x = (config.width - zoomedWidth) / 2.0;
        const double y = (config.height - zoomedHeight) / 2.0;
        const QPointF center(x + zoomedWidth / 2.0, y + zoomedHeight / 2.0);

        painter.setOpacity(config.imageOpacity);

        // Translate to center, rotate, then draw
        painter.translate(center);
        const double rotation = static_cast<double>(frameIndex) * config.imageRotationSpeed;
        painter.rotate(rotation); // rotate around center
        painter.translate(-center); // move back

        painter.drawImage(QRectF { x, y, zoomedWidth, zoomedHeight }, config.image);
    }
    painter.restore();
}

void renderLogo(const VideoConfig & config, QPainter & painter, size_t frameIndex)
{
    painter.save();
    if (!config.logo.isNull()) {
        const int x = config.logoX;
        const int y = config.logoY;
        painter.setOpacity(1.0 * std::pow(config.logoFadeFactor, frameIndex));
        painter.drawImage(QRect { x, y, config.logo.width(), config.logo.height() }, config.logo);
    }
    painter.restore();
}

void renderScrollingText(VideoGenerator::SongS song, const VideoConfig & config, QPainter & painter, double currentTimeMs)
{
    painter.save();
    if (!config.scrollingText.empty()) {
        const int trackHeight = config.height / static_cast<int>(song->trackCount());
        const auto scrollText = QString::fromStdString(config.scrollingText);
        const auto textHeight = trackHeight / 4;
        painter.setPen(Qt::white);
        auto font = QFont { "Arial", textHeight };
        font.setBold(true);
        font.setCapitalization(QFont::Capitalization::AllUppercase);
        painter.setOpacity(config.scrollingTextOpacity);
        painter.setFont(font);
        const auto textWidth = painter.fontMetrics().horizontalAdvance(scrollText);
        const auto lastTrackIndex = song->trackCount() - 1;
        const auto lastTrackY = static_cast<int>(lastTrackIndex) * trackHeight;
        const auto scrollSpeed = config.width / 10; // width / 10 pixels per second
        const auto scrollTextX = config.width - static_cast<int>((currentTimeMs / 1000.0) * scrollSpeed);
        const auto scrollTextY = lastTrackY + (trackHeight - textHeight) / 2;
        if (scrollTextX + textWidth >= 0) {
            painter.drawText(scrollTextX, scrollTextY + textHeight, scrollText);
        }
    }
    painter.restore();
}

void writeVideoFrame(const QImage & frame, const VideoConfig & config, size_t frameIndex)
{
    const auto outputDir = QString::fromStdString(config.outputDir);
    const auto filePath = QString { "%1/frame_%2.png" }.arg(outputDir).arg(frameIndex, 5, 10, QChar('0'));
    if (const auto success = frame.save(filePath); !success) {
        std::stringstream ss;
        ss << "Cannot write file " << std::quoted(filePath.toStdString());
        throw std::runtime_error(ss.str());
    }
}

void VideoGenerator::generateVideoFrame(SongS song, const VideoConfig & config, size_t frameIndex, double currentTimeMs)
{
    QImage frame { static_cast<int>(config.width), static_cast<int>(config.height), QImage::Format_ARGB32 };
    frame.fill("#222222");

    QPainter painter { &frame };
    painter.setRenderHint(QPainter::Antialiasing);

    renderImage(config, painter, frameIndex);
    renderTrackBackgrounds(song, config, painter);
    renderLogo(config, painter, frameIndex);
    renderScrollingText(song, config, painter, currentTimeMs);

    m_animation->renderAnimationFrame(painter, frameIndex, currentTimeMs);

    writeVideoFrame(frame, config, frameIndex);
}

VideoGenerator::~VideoGenerator() = default;

} // namespace noteahead
