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

#include "default_particle_animation.hpp"

#include "../../application/mixer_service.hpp"
#include "../../application/note_converter.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"
#include "video_config.hpp"

#include <iomanip>

#include <QPainter>
#include <QRandomGenerator>

namespace noteahead {

static const auto TAG = "DefaultParticleAnimation";

DefaultParticleAnimation::DefaultParticleAnimation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick)
  : Animation { song, config, mixerService, minTick, maxTick }
{
}

void DefaultParticleAnimation::integrate(AnimationFrame & animationFrame, double dt, double floor)
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

DefaultParticleAnimation::AnimationFrame::ParticleList DefaultParticleAnimation::createSecondaryParticles(double x, double y, int note) const
{
    DefaultParticleAnimation::AnimationFrame::ParticleList particles;
    for (int i = 0; i < 5; i++) {
        AnimationFrame::Particle particle;
        particle.role = AnimationFrame::Particle::Role::Sparkle;
        particle.x = x + QRandomGenerator::global()->bounded(10) - 5;
        particle.y = y;
        particle.aY = 90 + QRandomGenerator::global()->bounded(10);
        particle.r = 0.125;
        particle.midiNote = note;
        particle.a = 0.9995;
        juzzlin::L(TAG).debug() << "x: " << particle.x << " y: " << particle.y << " r: " << particle.r;
        particles.push_back(particle);
    }
    return particles;
}

void DefaultParticleAnimation::generateAnimationFrames(const EventMap & events)
{
    juzzlin::L(TAG).info() << "Generating animation frames..";

    const int numNotes = 88;
    const int numTracks = static_cast<int>(song()->trackIndices().size());

    AnimationFrameS previousFrame = std::make_shared<AnimationFrame>();

    const double tickDurationS = 60 / (static_cast<double>(song()->beatsPerMinute() * song()->linesPerBeat() * song()->ticksPerLine()));

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
        integrate(*newAnimationFrame, tickDurationS, config().height);
        // Remove small-radius particles
        newAnimationFrame->particles.erase(std::ranges::remove_if(newAnimationFrame->particles, [](const auto & particle) { return particle.r < 0.001; }).begin(), newAnimationFrame->particles.end());

        // Create new particles for new notes
        if (auto && eventsAtTick = events.find(tick); eventsAtTick != events.end()) {
            for (auto && event : eventsAtTick->second) {
                if (shouldEventPlay(*event)) {
                    if (auto && noteData = event->noteData(); noteData) {
                        if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                            const double effectiveVelocity = mixerService()->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                            const auto note = *event->noteData()->note() - 21; // Map MIDI note to 0-87 range
                            if (const auto track = song()->trackPositionByIndex(event->noteData()->track()); track.has_value()) {
                                AnimationFrame::Particle particle;
                                particle.role = AnimationFrame::Particle::Role::Note;
                                const auto noteParticleX = note * config().width / numNotes + config().width / numNotes / 2;
                                particle.x = noteParticleX;
                                const auto noteParticleY = static_cast<int>(*track) * config().height / numTracks + config().height / numTracks / 2;
                                particle.y = noteParticleY;
                                particle.r = effectiveVelocity / 127;
                                particle.midiNote = note;
                                particle.a = 0.995;
                                juzzlin::L(TAG).debug() << "x: " << particle.x << " y: " << particle.y << " r: " << particle.r;
                                newAnimationFrame->particles.push_back(particle);
                                std::ranges::copy(createSecondaryParticles(particle.x, particle.y, note), std::back_inserter(newAnimationFrame->particles));

                                if (flashTrack.has_value() && *flashTrack == event->noteData()->track()) {
                                    if (!flashColumn.has_value() || *flashColumn == event->noteData()->column()) {
                                        AnimationFrame::Particle particle;
                                        particle.role = AnimationFrame::Particle::Role::Flash;
                                        particle.x = config().width * 0.5;
                                        particle.y = config().height * 0.5;
                                        particle.r = 1.0;
                                        particle.a = 0.995;
                                        juzzlin::L(TAG).debug() << "x: " << particle.x << " y: " << particle.y << " r: " << particle.r;
                                        newAnimationFrame->particles.push_back(particle);
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

void DefaultParticleAnimation::renderAnimationFrame(QPainter & painter, size_t frameIndex, double currentTimeMs)
{
    const double tickDurationMs = 60'000.0 / (static_cast<double>(song()->beatsPerMinute() * song()->linesPerBeat() * song()->ticksPerLine()));
    const size_t currentTick = minTick() + static_cast<size_t>((currentTimeMs - static_cast<double>(config().leadInTime.count())) / tickDurationMs);
    if (currentTick <= maxTick()) {
        if (const auto it = m_animationFrames.find(currentTick); it != m_animationFrames.end()) {
            juzzlin::L(TAG).info() << "Generating video frame " << frameIndex << " of animation frame " << currentTick;
            const auto maxRadius = config().height / static_cast<int>(song()->trackCount() + 1) / 3;
            for (auto && particle : it->second->particles) {
                const auto color = noteColor(particle.midiNote);
                if (particle.role == AnimationFrame::Particle::Role::Sparkle) {
                    painter.save();
                    painter.setPen(color);
                    painter.setBrush(color);
                    const auto radius = static_cast<int>(particle.r * maxRadius); // Scale radius
                    painter.drawEllipse(QPoint { static_cast<int>(particle.x), static_cast<int>(particle.y) }, radius, radius);
                    painter.restore();
                } else if (particle.role == AnimationFrame::Particle::Role::Note) {
                    if (const int textSize = static_cast<int>(particle.r * maxRadius * 2); textSize >= 1) {
                        painter.save();
                        painter.setPen(Qt::white);
                        QFont font { "monospace", textSize };
                        font.setBold(true);
                        font.setCapitalization(QFont::Capitalization::AllUppercase);
                        painter.setFont(font);
                        const auto noteText = QString::fromStdString(NoteConverter::midiToString(particle.midiNote + 21));
                        QFontMetrics metrics(font);
                        const QRect textRect = metrics.boundingRect(noteText);
                        const int textX = static_cast<int>(particle.x) - textRect.width() / 2;
                        const int textY = static_cast<int>(particle.y) - textRect.height() / 2;
                        painter.drawText(textX, textY, noteText);
                        painter.restore();
                    }
                } else if (particle.role == AnimationFrame::Particle::Role::Flash) {
                    painter.save();
                    painter.setOpacity(particle.r);
                    painter.fillRect(QRect { 0, 0, config().width, config().height }, Qt::white);
                    painter.restore();
                }
            }
        } else {
            juzzlin::L(TAG).warning() << "Animation frame for video frame " << frameIndex << " not found at tick " << currentTick;
        }
    }
}

} // namespace noteahead
