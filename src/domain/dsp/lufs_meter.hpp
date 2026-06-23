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

#ifndef LUFS_METER_HPP
#define LUFS_METER_HPP

#include "../effects/effect.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

class LufsMeter : public Effect
{
public:
    LufsMeter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

    float momentaryLufs() const;
    float shortTermLufs() const;

private:
    void updateCoefficients();
    double applyKWeightL(double x);
    double applyKWeightR(double x);
    void advanceBlock(double meanPower);

    // K-weighting biquad coefficients (stage 1: high-shelf, stage 2: high-pass)
    double m_b0s1 { 1.0 }, m_b1s1 { 0.0 }, m_b2s1 { 0.0 };
    double m_a1s1 { 0.0 }, m_a2s1 { 0.0 };
    double m_b0s2 { 1.0 }, m_b1s2 { 0.0 }, m_b2s2 { 0.0 };
    double m_a1s2 { 0.0 }, m_a2s2 { 0.0 };

    // Biquad delay-line state per channel and stage (transposed direct form II)
    double m_z1s1L { 0 }, m_z2s1L { 0 }, m_z1s2L { 0 }, m_z2s2L { 0 };
    double m_z1s1R { 0 }, m_z2s1R { 0 }, m_z1s2R { 0 }, m_z2s2R { 0 };

    // 100-ms block accumulation
    size_t m_blockSize { 4800 };
    size_t m_blockSamples { 0 };
    double m_blockPowerSum { 0.0 };

    // Circular buffer of 100-ms block mean powers (30 blocks = 3 seconds)
    static constexpr size_t NumBlocks = 30;
    std::array<double, NumBlocks> m_blocks {};
    size_t m_blockWriteIdx { 0 };
    size_t m_blocksValid { 0 };

    float m_momentaryLufs { -70.0f };
    float m_shortTermLufs { -70.0f };

    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // LUFS_METER_HPP
