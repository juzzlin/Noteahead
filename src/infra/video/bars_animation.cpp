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

#include "bars_animation.hpp"

#include "../../application/service/mixer_service.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"
#include "video_config.hpp"

#include <iomanip>

#include <QPainter>
#include <QRandomGenerator>

namespace noteahead {

static const auto TAG = "BarsAnimation";

static const int NOTE_COUNT = 88;

BarsAnimation::BarsAnimation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick)
  : Animation { song, config, mixerService, minTick, maxTick }
{
}

void BarsAnimation::integrate(AnimationFrame & animationFrame)
{
    // See DefaultAnimation::integrate() for a complete integration example.
    for (auto && particle : animationFrame.particles) {
        particle.r *= particle.a;
    }
}

BarsAnimation::AnimationFrame::Particle BarsAnimation::createNoteParticle(double x, double y, int note, double velocity, size_t track) const
{
    AnimationFrame::Particle particle;
    particle.role = AnimationFrame::Particle::Role::Note;
    particle.x = x;
    particle.y = y;
    particle.r = velocity;
    particle.midiNote = note;
    particle.a = 0.997;
    particle.track = track;
    return particle;
}

BarsAnimation::AnimationFrame::Particle BarsAnimation::createFlashParticle() const
{
    AnimationFrame::Particle particle;
    particle.role = AnimationFrame::Particle::Role::Flash;
    particle.x = config().width * 0.5;
    particle.y = config().height * 0.5;
    particle.r = 1.0;
    particle.a = 0.995;
    return particle;
}

void BarsAnimation::generateAnimationFrames(const EventMap & events)
{
    juzzlin::L(TAG).info() << "Generating animation frames..";

    AnimationFrameS previousFrame = std::make_shared<AnimationFrame>();

    std::optional<size_t> flashTrack;
    std::optional<size_t> flashColumn;
    if (config().flashTrackName.has_value()) {
        flashTrack = song()->trackByName(*config().flashTrackName);
        if (!flashTrack.has_value()) {
            std::stringstream ss;
            ss << "No such track name: " << std::quoted(config().flashTrackName.value());
            throw std::runtime_error(ss.str());
        } else {
            juzzlin::L(TAG).info() << "Flash track index: " << *flashTrack;
            if (config().flashColumnName.has_value()) {
                flashColumn = song()->columnByName(*flashTrack, *config().flashColumnName);
                if (!flashColumn.has_value()) {
                    std::stringstream ss;
                    ss << "No such column name: " << std::quoted(config().flashColumnName.value());
                    throw std::runtime_error(ss.str());
                } else {
                    juzzlin::L(TAG).info() << "Flash column index: " << *flashColumn;
                }
            }
        }
    }

    for (auto tick = minTick(); tick <= maxTick(); tick++) {
        const auto newAnimationFrame = std::make_shared<AnimationFrame>(*previousFrame);
        // Integrate previous frame
        integrate(*newAnimationFrame);
        // Remove old particles
        newAnimationFrame->particles.erase(std::ranges::remove_if(newAnimationFrame->particles, [](const auto & particle) { return particle.t > 15; }).begin(), newAnimationFrame->particles.end());

        // Create new particles for new notes
        if (auto && eventsAtTick = events.find(tick); eventsAtTick != events.end()) {
            for (auto && event : eventsAtTick->second) {
                if (shouldEventPlay(*event)) {
                    if (auto && noteData = event->noteData(); noteData) {
                        if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                            const auto note = *event->noteData()->note() - 21; // Map MIDI note to 0-87 range
                            if (const auto trackPosition = song()->trackPositionByIndex(noteData->track()); trackPosition.has_value()) {
                                const auto noteParticleX = note * config().width / NOTE_COUNT + config().width / NOTE_COUNT / 2;
                                const auto noteParticleY = 0;
                                const double effectiveVelocity = static_cast<double>(mixerService()->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity())) / 127;
                                // Remove same notes on same track to clean up note mess
                                newAnimationFrame->particles.erase(std::ranges::remove_if(newAnimationFrame->particles, [&](const auto & particle) { return particle.midiNote == note && particle.track == *trackPosition; }).begin(), newAnimationFrame->particles.end());
                                newAnimationFrame->particles.push_back(createNoteParticle(noteParticleX, noteParticleY, note, effectiveVelocity, *trackPosition));
                                if (flashTrack.has_value() && *flashTrack == noteData->track()) {
                                    if (!flashColumn.has_value() || *flashColumn == event->noteData()->column()) {
                                        newAnimationFrame->particles.push_back(createFlashParticle());
                                    }
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

void BarsAnimation::renderNoteParticle(QPainter & painter, AnimationFrame::Particle & particle) const
{
    if (particle.r > 0.001) {
        painter.save();
        const double barWidth = particle.r * config().width / NOTE_COUNT;
        painter.setPen(Qt::white);
        painter.setOpacity(2.0 * std::pow(particle.r, 2));
        painter.fillRect(QRectF { particle.x - barWidth / 2, 0, barWidth, static_cast<double>(config().height) }, Qt::white);
        painter.restore();
    }
}

void BarsAnimation::renderFlashParticle(QPainter & painter, AnimationFrame::Particle & particle) const
{
    painter.save();
    painter.setOpacity(particle.r);
    painter.fillRect(QRect { 0, 0, config().width, config().height }, Qt::white);
    painter.restore();
}

void BarsAnimation::renderParticles(QPainter & painter, AnimationFrame & animationFrame) const
{
    for (auto && particle : animationFrame.particles) {
        if (particle.role == AnimationFrame::Particle::Role::Note) {
            renderNoteParticle(painter, particle);
        } else if (particle.role == AnimationFrame::Particle::Role::Flash) {
            renderFlashParticle(painter, particle);
        }
    }
}

void BarsAnimation::renderAnimationFrame(QPainter & painter, size_t frameIndex, double currentTimeMs)
{
    const double tickDurationMs = 60'000.0 / (static_cast<double>(song()->beatsPerMinute() * song()->linesPerBeat() * song()->ticksPerLine()));
    const size_t currentTick = minTick() + static_cast<size_t>((currentTimeMs - static_cast<double>(config().leadInTime.count())) / tickDurationMs);
    if (currentTick <= maxTick()) {
        if (const auto it = m_animationFrames.find(currentTick); it != m_animationFrames.end()) {
            juzzlin::L(TAG).info() << "Generating video frame " << frameIndex << " of animation frame " << currentTick;
            renderParticles(painter, *it->second);
        } else {
            juzzlin::L(TAG).warning() << "Animation frame for video frame " << frameIndex << " not found at tick " << currentTick;
        }
    }
}

} // namespace noteahead
