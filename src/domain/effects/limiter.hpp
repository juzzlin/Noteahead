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
    // Monotonic decreasing deque for the sliding-window peak maximum, as a fixed-capacity ring.
    // A std::deque frees a node on pop_front and allocates one on push_back as the window crosses
    // its chunk boundaries, which here would mean allocating on the audio thread while limiting.
    // The window holds at most one entry per delay-buffer slot, so m_peakBuffer's size is capacity.
    std::vector<uint32_t> m_maxIndices;
    uint32_t m_maxHead { 0 };
    uint32_t m_maxCount { 0 };

    void maxIndicesClear();
    bool maxIndicesEmpty() const;
    uint32_t maxIndicesFront() const;
    uint32_t maxIndicesBack() const;
    void maxIndicesPopFront();
    void maxIndicesPopBack();
    void maxIndicesPushBack(uint32_t value);

    uint32_t m_writePos { 0 };
    uint32_t m_delaySamples { 0 };

    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // LIMITER_HPP
