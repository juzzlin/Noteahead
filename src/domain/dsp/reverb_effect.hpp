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

#ifndef REVERB_EFFECT_HPP
#define REVERB_EFFECT_HPP

#include "../devices/effect.hpp"
#include <vector>
#include <array>

namespace noteahead {

class ReverbEffect : public Effect
{
public:
    enum class Preset {
        Hall,
        LargeRoom,
        SmallRoom,
        Plate,
        Cathedral,
        Basement,
        Tunnel,
        Spring
    };

    ReverbEffect();

    std::string type() const override { return "reverb"; }
    void process(float & left, float & right) override;
    void reset() override;
    void sync() override;

    void setSize(float size);
    float size() const;

    void setDecay(float decay);
    float decay() const;

    void setDamping(float damping);
    float damping() const;

    void setPreDelay(float ms);
    float preDelay() const;

    void setMix(float mix);
    float mix() const;

    void setWidth(float width);
    float width() const;

    void applyPreset(Preset preset);

private:
    void syncParameters();
    void updateBuffers();

    float m_size { 0.5f };
    float m_decayMs { 1500.0f };
    float m_damping { 0.3f };
    float m_preDelayMs { 20.0f };
    float m_mix { 0.0f };
    float m_width { 1.0f };

    static constexpr int NumDelays = 4;
    
    struct DelayLine {
        std::vector<float> buffer;
        uint32_t writePos { 0 };
        float lpState { 0.0f };
        uint32_t size { 0 };
        float feedback { 0.0f };

        void resize(uint32_t n) {
            buffer.assign(n, 0.0f);
            size = n;
            writePos = 0;
            lpState = 0.0f;
        }

        float process(float input, float damping) {
            float output = buffer[writePos];
            // Low-pass absorption
            lpState = output + damping * (lpState - output);
            buffer[writePos] = input * feedback;
            if (++writePos >= size) writePos = 0;
            return lpState;
        }

        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            lpState = 0.0f;
        }
    };

    std::array<DelayLine, NumDelays> m_delays;
    std::vector<float> m_preDelayBuffer;
    uint32_t m_preDelayWritePos { 0 };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // REVERB_EFFECT_HPP
