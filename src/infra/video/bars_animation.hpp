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

#ifndef BARS_ANIMATION_H
#define BARS_ANIMATION_H

#include "animation.hpp"

#include <map>

namespace noteahead {

class BarsAnimation : public Animation
{
public:
    explicit BarsAnimation(SongS song, const VideoConfig & config, MixerServiceS mixerService, size_t minTick, size_t maxTick);

    void generateAnimationFrames(const EventMap & events) override;

    void renderAnimationFrame(QPainter & painter, size_t frameIndex, double currentTimeMs) override;

private:
    void integrate(AnimationFrame & animationFrame);

    BarsAnimation::AnimationFrame::Particle createFlashParticle() const;
    BarsAnimation::AnimationFrame::Particle createNoteParticle(double x, double y, int note, double velocity, size_t track) const;
    BarsAnimation::AnimationFrame::Particle createPrimaryParticle(double x, double y, int note, double velocity) const;

    void renderParticles(QPainter & painter, AnimationFrame & animationFrame) const;
    void renderNoteParticle(QPainter & painter, AnimationFrame::Particle & particle) const;
    void renderFlashParticle(QPainter & painter, AnimationFrame::Particle & particle) const;

    using AnimationFrameS = std::shared_ptr<AnimationFrame>;
    using TickToAnimationFrameMap = std::map<size_t, AnimationFrameS>;
    TickToAnimationFrameMap m_animationFrames;

    std::unique_ptr<Animation> m_animation;
};

} // namespace noteahead

#endif // BARS_ANIMATION_H
