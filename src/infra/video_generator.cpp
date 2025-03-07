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
    const auto [minTick, maxTick] = std::ranges::minmax(m_eventMap | std::views::keys);
    m_minTick = minTick;
    m_maxTick = maxTick;
    juzzlin::L(TAG).info() << "Min tick: " << m_minTick;
    juzzlin::L(TAG).info() << "Max tick: " << m_maxTick;
}

bool VideoGenerator::shouldEventPlay(const Event & event) const
{
    if (auto && noteData = event.noteData(); noteData) {
        return m_mixerService->shouldColumnPlay(noteData->track(), noteData->column());
    }
    return true;
}

void VideoGenerator::generateAnimationFrames(SongS song, const Config & config)
{
    juzzlin::L(TAG).info() << "Generating animation frames..";

    const int numNotes = 88;
    const int numTracks = static_cast<int>(song->trackIndices().size());

    AnimationFrameS previousFrame = std::make_shared<AnimationFrame>();

    const double attenuation = 0.99;

    for (auto tick = m_minTick; tick <= m_maxTick; tick++) {
        const auto newAnimationFrame = std::make_shared<AnimationFrame>(*previousFrame);
        // Animate previous frame
        for (auto && track : newAnimationFrame->trackToItemsMap) {
            for (auto && item : track.second) {
                item.r *= attenuation;
            }
        }
        // Remove small-radius items
        for (auto & [track, items] : newAnimationFrame->trackToItemsMap) {
            items.erase(std::ranges::remove_if(items, [](const auto & item) { return item.r < 0.1; }).begin(), items.end());
        }
        // Create new items for new notes
        if (auto && eventsAtTick = m_eventMap.find(tick); eventsAtTick != m_eventMap.end()) {
            for (auto && event : eventsAtTick->second) {
                if (shouldEventPlay(*event)) {
                    if (auto && noteData = event->noteData(); noteData) {
                        if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                            const double effectiveVelocity = m_mixerService->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                            const auto note = *event->noteData()->note() - 21; // Map MIDI note to 0-87 range
                            if (const auto track = song->trackPositionByIndex(event->noteData()->track()); track.has_value()) {
                                AnimationFrame::Item item;
                                item.x = note * config.width / numNotes + config.width / numNotes / 2;
                                item.y = static_cast<int>(*track) * config.height / numTracks + config.height / numTracks / 2;
                                item.r = effectiveVelocity / 127;
                                item.midiNote = note;
                                juzzlin::L(TAG).debug() << "x: " << item.x << " y: " << item.y << " r: " << item.r;
                                newAnimationFrame->trackToItemsMap[event->noteData()->track()].push_back(item);
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
    if (m_song = song; !m_song) {
        return;
    }

    initialize(config);

    const auto fps = m_config.fps;
    const auto totalFrames = config.length.has_value()
      ? fps * static_cast<size_t>(config.length->count()) / 1'000
      : fps * static_cast<size_t>(song->duration(config.startPosition).count()) / 1'000;
    const double frameDurationMs = 1'000.0 / static_cast<double>(fps);

    generateAnimationFrames(song, config);

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

    const QImage overlayImage = !config.image.empty() ? QImage { QString::fromStdString(config.image) } : QImage {};

    // Limit concurrent threads using a dynamically allocated semaphore
    const auto maxThreads = std::max(1u, std::thread::hardware_concurrency()); // At least 1 thread
    const auto semaphore = std::make_unique<std::counting_semaphore<>>(maxThreads);
    std::vector<std::future<void>> futures;
    for (size_t frameIndex = 0; frameIndex < totalFrames; frameIndex++) {
        semaphore->acquire(); // Wait for a thread slot to be available
        futures.push_back(std::async(std::launch::async, [&, frameIndex]() {
            const double currentTimeMs = frameDurationMs * static_cast<double>(frameIndex);
            generateVideoFrame(song, config, overlayImage, frameIndex, currentTimeMs);
            juzzlin::L(TAG).info() << "Frame " << frameIndex + 1 << "/" << totalFrames; // SimpleLogger is thread-safe
            semaphore->release(); // Release the thread slot when done
        }));
    }

    // Ensure all tasks complete before finishing
    for (auto & future : futures) {
        future.get();
    }

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

void VideoGenerator::generateVideoFrame(SongS song, const Config & config, QImage overlayImage, size_t frameIndex, double currentTimeMs)
{
    const double tickDurationMs = 60'000.0 / (static_cast<double>(song->beatsPerMinute() * song->linesPerBeat() * song->ticksPerLine()));
    size_t currentTick = static_cast<size_t>(currentTimeMs / tickDurationMs);

    QImage frame { static_cast<int>(config.width), static_cast<int>(config.height), QImage::Format_ARGB32 };
    frame.fill("#222222");
    QPainter painter { &frame };
    painter.setRenderHint(QPainter::Antialiasing);

    static const std::vector<QString> trackColors = {
        "#202020", "#242424"
    };

    // Paint track backgrounds
    const int trackHeight = config.height / static_cast<int>(song->trackCount());
    for (size_t trackIndex = 0; trackIndex < song->trackCount(); trackIndex++) {
        const QColor trackColor { trackColors.at(trackIndex % trackColors.size()) };
        QRect trackRect { 0, static_cast<int>(trackIndex) * trackHeight, static_cast<int>(config.width), static_cast<int>(trackHeight) };
        painter.fillRect(trackRect, trackColor);
    }

    // Overlay fixed image
    if (!overlayImage.isNull()) {
        const int imageHeight = config.height;
        const double aspectRatio = static_cast<double>(overlayImage.width()) / overlayImage.height();
        const int imageWidth = static_cast<int>(imageHeight * aspectRatio);
        const int x = (config.width - imageWidth) / 2;
        const int y = 0;
        painter.setOpacity(0.25);
        painter.drawImage(QRect { x, y, imageWidth, imageHeight }, overlayImage);
        painter.setOpacity(1.0);
    }

    // Scrolling text setup
    if (!config.scrollingText.empty()) {
        const auto scrollText = QString::fromStdString(config.scrollingText);
        const auto textHeight = trackHeight / 2;
        painter.setPen(Qt::white);
        auto font = QFont { "Arial", textHeight };
        font.setBold(true);
        font.setCapitalization(QFont::Capitalization::AllUppercase);
        painter.setFont(font);
        const auto textWidth = painter.fontMetrics().horizontalAdvance(scrollText);
        const auto lastTrackIndex = song->trackCount() - 1;
        const auto lastTrackY = static_cast<int>(lastTrackIndex) * trackHeight;
        const auto scrollSpeed = 100; // pixels per second
        int scrollX = config.width - static_cast<int>((currentTimeMs / 1000.0) * scrollSpeed);
        if (scrollX + textWidth < 0) {
            scrollX += textWidth + config.width;
        }
        const auto scrollY = lastTrackY + (trackHeight - textHeight) / 2;
        painter.drawText(scrollX, scrollY + textHeight, scrollText);
    }

    if (const auto it = m_animationFrames.find(currentTick); it != m_animationFrames.end()) {
        juzzlin::L(TAG).info() << "Generating video frame " << frameIndex << " of animation frame " << currentTick;
        const auto maxRadius = config.height / static_cast<int>((song->trackCount() + 1)) / 2;
        for (const auto & [track, items] : it->second->trackToItemsMap) {
            for (const auto & item : items) {
                QColor color = noteColor(item.midiNote);
                painter.setPen(color);
                painter.setBrush(color);
                const auto radius = static_cast<int>(item.r * maxRadius); // Scale radius
                painter.drawEllipse(QPoint { item.x, item.y }, radius, radius);
            }
        }
    } else {
        juzzlin::L(TAG).warning() << "Animation frame for video frame " << frameIndex << " not found at tick " << currentTick;
    }

    const auto outputDir = QString::fromStdString(config.outputDir);
    if (QDir dir { outputDir }; !dir.exists()) {
        juzzlin::L(TAG).info() << "Creating '" << outputDir.toStdString() << "'";
        dir.mkpath(outputDir);
    }

    const auto filePath = QString { "%1/frame_%2.png" }.arg(outputDir).arg(frameIndex, 5, 10, QChar('0'));
    if (const auto success = frame.save(filePath); !success) {
        throw std::runtime_error(
          std::string {
            "Cannot write file '" + filePath.toStdString() + "'" });
    }
}

} // namespace noteahead
