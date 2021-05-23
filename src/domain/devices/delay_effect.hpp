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

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void setSampleRate(double sampleRate) override;
    void reset() override;

    void setType(Type type);
    void setTime(double seconds);
    void setFeedback(double feedback);
    void setDepth(double depth);
    void setMix(double mix);
    void setBpm(double bpm);
    double bpm() const;
    void setSync(bool sync);
    void setSyncDivision(double division);

    void setFeedbackLpf(double cutoff);
    double feedbackLpf() const;
    void setFeedbackHpf(double cutoff);
    double feedbackHpf() const;

private:
    double calculateDelaySamples() const;
    double readFromBuffer(const std::vector<double> & buffer, double delay) const;
    void applyFeedbackFilters(double & fbL, double & fbR);
    void applyMix(double & left, double & right, double outL, double outR) const;
    void applyTapeSaturation(double & fbL, double & fbR);
    void updateBuffers(uint32_t sampleRate);
    void updateFilters();
    void updateWriteBuffer(double inputL, double inputR, double fbL, double fbR, double & outL, double & outR);

    Type m_type { Type::Stereo };
    double m_time { 0.5 };
    double m_feedback { 0.3 };
    double m_depth { 0.5 };
    double m_mix { 0.0 };
    double m_bpm { 120.0 };
    bool m_sync { false };
    double m_syncDivision { 0.25 };
    double m_feedbackLpfCutoff { 1.0 };
    double m_feedbackHpfCutoff { 0.0 };

    std::vector<double> m_bufferL;
    std::vector<double> m_bufferR;
    uint32_t m_writePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    // Feedback filters
    CascadedSvf m_fbLpfL;
    CascadedSvf m_fbLpfR;
    CascadedSvf m_fbHpfL;
    CascadedSvf m_fbHpfR;

    // Filter states for legacy HiPass/LowPass/Tape
    double m_lpStateL { 0.0 };
    double m_lpStateR { 0.0 };
    double m_hpStateL { 0.0 };
    double m_hpStateR { 0.0 };
};

} // namespace noteahead

#endif // DELAY_EFFECT_HPP
