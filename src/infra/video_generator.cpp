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

#include "../application/mixer_service.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "../domain/note_data.hpp"

#include <algorithm>
#include <future>
#include <ranges>
#include <semaphore>

#include <QDir>
#include <QPainter>
#include <QProcess>
#include <QRandomGenerator>

namespace noteahead {

static const auto TAG = "VideoGenerator";

VideoGenerator::VideoGenerator(MixerServiceS mixerService)
  : m_mixerService { mixerService }
{
}

void VideoGenerator::initialize(const Config & config)
{
    m_config = config;
    m_eventMap.clear();
    m_allInstruments.clear();
    for (auto && event : m_song->renderToEvents(config.startPosition)) {
        m_eventMap[event->tick()].push_back(event);
        if (event->instrument()) {
            m_allInstruments.insert(event->instrument());
        }
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

bool VideoGenerator::shouldEventPlay(const Event & event) const
{
    if (auto && noteData = event.noteData(); noteData) {
        return m_mixerService->shouldColumnPlay(noteData->track(), noteData->column());
    }
    return true;
}

void VideoGenerator::integrate(AnimationFrame & animationFrame, double dt, double floor)
{
    for (auto && particle : animationFrame.particles) {
        particle.r *= particle.a;
        particle.vX += particle.aX * dt;
        particle.vY += particle.aY * dt;
        particle.x += particle.vX * dt;
        particle.y += particle.vY * dt;
        if (particle.y > floor) {
            particle.y = floor;
            particle.vY = -particle.vY * 0.5;
            particle.vX += particle.vY * 0.5 * (QRandomGenerator::global()->bounded(100) - 50) / 100;
        }
    }
}

void VideoGenerator::generateAnimationFrames(SongS song, const Config & config)
{
    juzzlin::L(TAG).info() << "Generating animation frames..";

    const int numNotes = 88;
    const int numTracks = static_cast<int>(song->trackIndices().size());

    AnimationFrameS previousFrame = std::make_shared<AnimationFrame>();

    const double tickDurationS = 60 / (static_cast<double>(song->beatsPerMinute() * song->linesPerBeat() * song->ticksPerLine()));

    for (auto tick = m_minTick; tick <= m_maxTick; tick++) {
        const auto newAnimationFrame = std::make_shared<AnimationFrame>(*previousFrame);
        // Integrate previous frame
        integrate(*newAnimationFrame, tickDurationS, config.height);
        // Remove small-radius particles
        newAnimationFrame->particles.erase(std::ranges::remove_if(newAnimationFrame->particles, [](const auto & particle) { return particle.r < 0.001; }).begin(), newAnimationFrame->particles.end());

        // Create new particles for new notes
        if (auto && eventsAtTick = m_eventMap.find(tick); eventsAtTick != m_eventMap.end()) {
            for (auto && event : eventsAtTick->second) {
                if (shouldEventPlay(*event)) {
                    if (auto && noteData = event->noteData(); noteData) {
                        if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                            const double effectiveVelocity = m_mixerService->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                            const auto note = *event->noteData()->note() - 21; // Map MIDI note to 0-87 range
                            if (const auto track = song->trackPositionByIndex(event->noteData()->track()); track.has_value()) {
                                AnimationFrame::Particle particle;
                                const auto noteParticleX = note * config.width / numNotes + config.width / numNotes / 2;
                                particle.x = noteParticleX;
                                const auto noteParticleY = static_cast<int>(*track) * config.height / numTracks + config.height / numTracks / 2;
                                particle.y = noteParticleY;
                                particle.r = effectiveVelocity / 127;
                                particle.midiNote = note;
                                particle.a = 0.99;
                                juzzlin::L(TAG).debug() << "x: " << particle.x << " y: " << particle.y << " r: " << particle.r;
                                newAnimationFrame->particles.push_back(particle);

                                for (int i = 0; i < 5; i++) {
                                    AnimationFrame::Particle particle;
                                    particle.x = noteParticleX + QRandomGenerator::global()->bounded(10) - 5;
                                    particle.y = noteParticleY;
                                    particle.aY = 90 + QRandomGenerator::global()->bounded(10);
                                    particle.r = 0.125;
                                    particle.midiNote = note;
                                    particle.a = 0.999;
                                    juzzlin::L(TAG).debug() << "x: " << particle.x << " y: " << particle.y << " r: " << particle.r;
                                    newAnimationFrame->particles.push_back(particle);
                                }
                            }
                        }
                    }
                }
            }
        }

        m_animationFrames[tick] = newAnimationFrame;
        previousFrame = newAnimationFrame;
    }
}

void VideoGenerator::generateVideoFrames(SongS song, const Config & config)
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
    juzzlin::L(TAG).debug() << "Writing frames to '" << outputDir.toStdString() << "'";
    if (QDir dir { outputDir }; !dir.exists()) {
        juzzlin::L(TAG).info() << "Creating '" << outputDir.toStdString() << "'";
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

void VideoGenerator::renderVideo(const Config & config)
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
        QString::fromStdString(config.songPath) + ".youtube.mp4"
    };

    runCommand(ffmpegArgs);
}

void VideoGenerator::run(SongS song, const Config & config)
{
    if (m_song = song; !m_song) {
        return;
    }

    initialize(config);

    generateAnimationFrames(song, config);

    generateVideoFrames(song, config);

    renderVideo(config);

    juzzlin::L(TAG).info() << "Done.";
}

QColor noteColor(int midiNote)
{
    const int minNote = 21;
    const int maxNote = 108;
    const int numKeys = maxNote - minNote + 1;

    double normalized = static_cast<double>(midiNote - minNote) / (numKeys - 1);
    double red = 1.0;
    double green = 2.0 * normalized;
    if (green > 1.0) {
        red = 2.0 - 2.0 * normalized;
        green = 1.0;
    }

    return QColor::fromRgbF(static_cast<float>(red), static_cast<float>(green), 0, 1);
}

void renderTrackBackgrounds(VideoGenerator::SongS song, const VideoGenerator::Config & config, QPainter & painter)
{
    static const std::vector<QString> trackColors = {
        "#202020", "#242424"
    };
    const int trackHeight = config.height / static_cast<int>(song->trackCount());
    for (size_t trackIndex = 0; trackIndex < song->trackCount(); trackIndex++) {
        const QColor trackColor { trackColors.at(trackIndex % trackColors.size()) };
        QRect trackRect { 0, static_cast<int>(trackIndex) * trackHeight, static_cast<int>(config.width), static_cast<int>(trackHeight) };
        painter.fillRect(trackRect, trackColor);
    }
}

void renderImage(const VideoGenerator::Config & config, QPainter & painter)
{
    if (!config.image.isNull()) {
        const int imageHeight = config.height;
        const double aspectRatio = static_cast<double>(config.image.width()) / config.image.height();
        const int imageWidth = static_cast<int>(imageHeight * aspectRatio);
        const int x = (config.width - imageWidth) / 2;
        const int y = 0;
        painter.setOpacity(0.25);
        painter.drawImage(QRect { x, y, imageWidth, imageHeight }, config.image);
        painter.setOpacity(1.0);
    }
}

void renderLogo(const VideoGenerator::Config & config, QPainter & painter)
{
    if (!config.logo.isNull()) {
        const int x = config.logoX;
        const int y = config.logoY;
        painter.drawImage(QRect { x, y, config.logo.width(), config.logo.height() }, config.logo);
        painter.setOpacity(1.0);
    }
}

void renderScrollingText(VideoGenerator::SongS song, const VideoGenerator::Config & config, QPainter & painter, double currentTimeMs)
{
    if (!config.scrollingText.empty()) {
        const int trackHeight = config.height / static_cast<int>(song->trackCount());
        const auto scrollText = QString::fromStdString(config.scrollingText);
        const auto textHeight = trackHeight / 4;
        painter.setPen(Qt::white);
        auto font = QFont { "Arial", textHeight };
        font.setBold(true);
        font.setCapitalization(QFont::Capitalization::AllUppercase);
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
}

void VideoGenerator::renderAnimationFrame(const Config & config, QPainter & painter, size_t frameIndex, double currentTimeMs)
{
    const double tickDurationMs = 60'000.0 / (static_cast<double>(m_song->beatsPerMinute() * m_song->linesPerBeat() * m_song->ticksPerLine()));
    const size_t currentTick = m_minTick + static_cast<size_t>((currentTimeMs - static_cast<double>(config.leadInTime.count())) / tickDurationMs);
    if (currentTick <= m_maxTick) {
        if (const auto it = m_animationFrames.find(currentTick); it != m_animationFrames.end()) {
            juzzlin::L(TAG).info() << "Generating video frame " << frameIndex << " of animation frame " << currentTick;
            const auto maxRadius = config.height / static_cast<int>(m_song->trackCount() + 1) / 3;
            for (auto && particle : it->second->particles) {
                const auto color = noteColor(particle.midiNote);
                painter.setPen(color);
                painter.setBrush(color);
                const auto radius = static_cast<int>(particle.r * maxRadius); // Scale radius
                painter.drawEllipse(QPoint { static_cast<int>(particle.x), static_cast<int>(particle.y) }, radius, radius);
            }
        } else {
            juzzlin::L(TAG).warning() << "Animation frame for video frame " << frameIndex << " not found at tick " << currentTick;
        }
    }
}

void writeVideoFrame(const QImage & frame, const VideoGenerator::Config & config, size_t frameIndex)
{
    const auto outputDir = QString::fromStdString(config.outputDir);
    const auto filePath = QString { "%1/frame_%2.png" }.arg(outputDir).arg(frameIndex, 5, 10, QChar('0'));
    if (const auto success = frame.save(filePath); !success) {
        throw std::runtime_error(
          std::string {
            "Cannot write file '" + filePath.toStdString() + "'" });
    }
}

void VideoGenerator::generateVideoFrame(SongS song, const Config & config, size_t frameIndex, double currentTimeMs)
{
    QImage frame { static_cast<int>(config.width), static_cast<int>(config.height), QImage::Format_ARGB32 };
    frame.fill("#222222");

    QPainter painter { &frame };
    painter.setRenderHint(QPainter::Antialiasing);

    renderTrackBackgrounds(song, config, painter);
    renderImage(config, painter);
    renderLogo(config, painter);
    renderScrollingText(song, config, painter, currentTimeMs);
    renderAnimationFrame(config, painter, frameIndex, currentTimeMs);

    writeVideoFrame(frame, config, frameIndex);
}

} // namespace noteahead
