// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef DELAY_EFFECT_HPP
#define DELAY_EFFECT_HPP

#include "effect.hpp"
#include <vector>

namespace noteahead {

class DelayEffect : public Effect
{
public:
    enum class Type
    {
        Stereo,
        Mono,
        PingPong,
        HiPass,
        LowPass,
        Tape
    };

    DelayEffect();

    void process(float & left, float & right, uint32_t sampleRate) override;
    void reset() override;

    void setType(Type type);
    void setTime(float seconds);
    void setFeedback(float feedback);
    void setDepth(float depth);
    void setMix(float mix);
    void setBpm(float bpm);
    float bpm() const;
    void setSync(bool sync);
    void setSyncDivision(float division);

private:
    Type m_type { Type::Stereo };
    float m_time { 0.5f };
    float m_feedback { 0.3f };
    float m_depth { 0.5f };
    float m_mix { 0.0f };
    float m_bpm { 120.0f };
    bool m_sync { false };
    float m_syncDivision { 0.25f };

    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    uint32_t m_writePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    // Filter states for HiPass/LowPass/Tape
    float m_lpStateL { 0.0f };
    float m_lpStateR { 0.0f };
    float m_hpStateL { 0.0f };
    float m_hpStateR { 0.0f };

    void updateBuffers(uint32_t sampleRate);
};

} // namespace noteahead

#endif // DELAY_EFFECT_HPP
