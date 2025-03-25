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

#ifndef DEFAULT_PARTICLE_ANIMATION_HPP
#define DEFAULT_PARTICLE_ANIMATION_HPP

#include "animation.hpp"

#include <map>
#include <vector>

namespace noteahead {

class DefaultParticleAnimation : public Animation
{
public:
    explicit DefaultParticleAnimation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick);

    void generateAnimationFrames(const EventMap & events) override;

    void renderAnimationFrame(QPainter & painter, size_t frameIndex, double currentTimeMs) override;

private:
    struct AnimationFrame
    {
        struct Particle
        {
            enum class Role
            {
                Note,
                Sparkle,
                Flash
            };

            Role role = Role::Sparkle;

            double x = 0;
            double y = 0;
            double vX = 0;
            double vY = 0;
            double aX = 0;
            double aY = 0;
            double r = 1.0;
            double a = 0.99;
            double t = 0;

            int midiNote = 0;
        };

        using ParticleList = std::vector<Particle>;
        ParticleList particles;
    };

    void integrate(AnimationFrame & animationFrame, double dt, double floor);

    DefaultParticleAnimation::AnimationFrame::Particle createFlashParticle() const;
    DefaultParticleAnimation::AnimationFrame::Particle createNoteParticle(double x, double y, int note, double velocity) const;
    DefaultParticleAnimation::AnimationFrame::Particle createPrimaryParticle(double x, double y, int note, double velocity) const;
    AnimationFrame::ParticleList createSecondaryParticles(double x, double y, int note) const;

    void renderLinesBetweenParticlesOnSameTrack(QPainter & painter, AnimationFrame & animationFrame) const;
    void renderParticles(QPainter & painter, AnimationFrame & animationFrame) const;
    void renderSparkleParticle(QPainter & painter, AnimationFrame::Particle & particle) const;
    void renderNoteParticle(QPainter & painter, AnimationFrame::Particle & particle) const;
    void renderFlashParticle(QPainter & painter, AnimationFrame::Particle & particle) const;

    using AnimationFrameS = std::shared_ptr<AnimationFrame>;
    using TickToAnimationFrameMap = std::map<size_t, AnimationFrameS>;
    TickToAnimationFrameMap m_animationFrames;

    std::unique_ptr<Animation> m_animation;
};

} // namespace noteahead

#endif // DEFAULT_PARTICLE_ANIMATION_HPP
