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

#ifndef LIMITER_HPP
#define LIMITER_HPP

#include "effect.hpp"

#include <cstdint>
#include <deque>
#include <vector>

namespace noteahead {

class Limiter : public Effect
{
public:
    Limiter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    float reductionDb() const;

private:
    void updateBuffers();
    void updateCoefficients();
    void applyLimiter(double & left, double & right);
    void syncParameters();

    float m_thresholdDb { 0.0f };
    float m_ceilingDb { -0.3f };
    float m_releaseMs { 100.0f };
    float m_lookaheadMs { 5.0f };
    bool m_boost { false };

    double m_releaseCoeff { 0.0 };

    double m_envGain { 1.0 };
    double m_reductionDb { 0.0 };

    std::vector<double> m_delayBufferL;
    std::vector<double> m_delayBufferR;
    std::vector<double> m_peakBuffer;
    std::deque<uint32_t> m_maxIndices; // Monotonic decreasing deque for the sliding-window peak maximum.
    uint32_t m_writePos { 0 };
    uint32_t m_delaySamples { 0 };

    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // LIMITER_HPP
