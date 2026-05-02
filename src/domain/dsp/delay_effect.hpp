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

#include <vector>
#include <cstdint>

namespace noteahead {

class DelayEffect
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

    void setSampleRate(double sampleRate);
    void setType(Type type);
    void setTime(double seconds); // Manual time
    void setFeedback(double feedback); // 0.0 to 1.0
    void setMix(double mix); // 0.0 to 1.0
    void setBpm(double bpm);
    void setSync(bool sync);
    void setSyncDivision(double division); // e.g. 0.25 for 1/4 note

    void process(float & left, float & right);
    void reset();

private:
    Type m_type { Type::Stereo };
    double m_sampleRate { 44100.0 };
    double m_time { 0.5 };
    double m_feedback { 0.3 };
    double m_mix { 0.0 };
    double m_bpm { 120.0 };
    bool m_sync { false };
    double m_syncDivision { 0.25 };

    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    uint32_t m_writePos { 0 };

    double getDelaySamples() const;
    void updateBuffers();

    // Filters for feedback
    float m_lpStateL { 0.0f };
    float m_lpStateR { 0.0f };
    float m_hpStateL { 0.0f };
    float m_hpStateR { 0.0f };
};

} // namespace noteahead

#endif // DELAY_EFFECT_HPP
