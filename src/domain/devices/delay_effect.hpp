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

#include "../dsp/cascaded_svf.hpp"
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
        Tape
    };

    DelayEffect();

    static std::string typeIdString()
    {
        return "7c2e3d0a-4f6b-4b2a-8c1d-1a2b3c4d5e6f";
    }

    std::string type() const override
    {
        return "delay";
    }
    std::string typeId() const override
    {
        return typeIdString();
    }
    void process(float & left, float & right) override;
    void process(AudioContext & context) override;
    void setSampleRate(double sampleRate) override;
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

    void setFeedbackLpf(float cutoff);
    float feedbackLpf() const;
    void setFeedbackHpf(float cutoff);
    float feedbackHpf() const;

private:
    double calculateDelaySamples() const;
    float readFromBuffer(const std::vector<float> & buffer, double delay) const;
    void applyFeedbackFilters(float & fbL, float & fbR);
    void applyMix(float & left, float & right, float outL, float outR) const;
    void applyTapeSaturation(float & fbL, float & fbR);
    void updateBuffers(uint32_t sampleRate);
    void updateFilters();
    void updateWriteBuffer(float inputL, float inputR, float fbL, float fbR, float & outL, float & outR);

    Type m_type { Type::Stereo };
    float m_time { 0.5f };
    float m_feedback { 0.3f };
    float m_depth { 0.5f };
    float m_mix { 0.0f };
    float m_bpm { 120.0f };
    bool m_sync { false };
    float m_syncDivision { 0.25f };
    float m_feedbackLpfCutoff { 1.0f };
    float m_feedbackHpfCutoff { 0.0f };

    std::vector<float> m_bufferL;
    std::vector<float> m_bufferR;
    uint32_t m_writePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    // Feedback filters
    CascadedSvf m_fbLpfL;
    CascadedSvf m_fbLpfR;
    CascadedSvf m_fbHpfL;
    CascadedSvf m_fbHpfR;

    // Filter states for legacy HiPass/LowPass/Tape
    float m_lpStateL { 0.0f };
    float m_lpStateR { 0.0f };
    float m_hpStateL { 0.0f };
    float m_hpStateR { 0.0f };
};

} // namespace noteahead

#endif // DELAY_EFFECT_HPP
