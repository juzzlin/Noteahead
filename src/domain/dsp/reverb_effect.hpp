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
#include "cascaded_svf.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace noteahead {

class ReverbEffect : public Effect
{
public:
    enum class Preset
    {
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

    static std::string typeIdString()
    {
        return "47a2e2d0-1e5e-4f3a-9c6a-6a5b2d7e8f1a";
    }

    std::string type() const override
    {
        return "reverb";
    }

    std::string typeId() const override
    {
        return typeIdString();
    }

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
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

    void setLpfCutoff(float cutoff);
    float lpfCutoff() const;

    void setHpfCutoff(float cutoff);
    float hpfCutoff() const;

    void applyPreset(Preset preset);

    static std::string presetToString(Preset preset);
    static Preset stringToPreset(const std::string & presetName);
    static std::vector<std::string> presetNames();

private:
    void syncParameters();
    void updateBuffers();
    void updateFilters();
    void applyWetFilters(double & wetL, double & wetR);

    float m_size { 0.5f };
    float m_decayMs { 1500.0f };
    float m_damping { 0.3f };
    float m_preDelayMs { 20.0f };
    float m_mix { 0.0f };
    float m_width { 1.0f };
    float m_lpfCutoff { 0.85f };
    float m_hpfCutoff { 0.2f };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };

    static constexpr int NumDelays = 8;

    struct DelayLine
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        double lpState { 0.0 };
        uint32_t size { 0 };
        double feedback { 0.0 };
        CascadedSvf fbLpf;
        CascadedSvf fbHpf;

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            writePos = 0;
            lpState = 0.0;
            fbLpf.reset();
            fbHpf.reset();
        }
    };

    std::array<DelayLine, NumDelays> m_delays;
    std::vector<double> m_preDelayBuffer;
    uint32_t m_preDelayWritePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    CascadedSvf m_wetLpfL;
    CascadedSvf m_wetLpfR;
    CascadedSvf m_wetHpfL;
    CascadedSvf m_wetHpfR;
};

} // namespace noteahead

#endif // REVERB_EFFECT_HPP
