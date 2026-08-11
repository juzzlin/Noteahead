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
#include <atomic>
#include <cstdint>

namespace noteahead {

class LufsMeter : public Effect
{
public:
    LufsMeter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    float momentaryLufs() const;
    float shortTermLufs() const;
    //! Gated integrated loudness per ITU-R BS.1770-4, over everything measured since the last reset.
    float integratedLufs() const;

    //! Clear the meter from another thread. The readings blank immediately; the accumulated state is
    //! dropped by the audio thread at the next sample, so no state is touched from under it.
    void requestReset();

private:
    void updateCoefficients();
    double applyKWeightL(double x);
    double applyKWeightR(double x);
    void advanceBlock(double meanPower);
    void accumulateGatingBlock(double meanPower);
    void updateIntegrated();
    static size_t histogramBin(double lufs);

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

    // Integrated loudness. The gated measurement needs every 400-ms gating block the song has ever
    // produced, which cannot be a growing list on the audio thread, so the blocks go into a fixed
    // histogram instead: one bin per 0.1 LU. Counts alone would quantise the answer, so each bin also
    // carries the exact sum of the powers filed under it — only the gate boundary is then quantised,
    // never the average, which keeps this within a hundredth of a dB of the offline LoudnessAnalyzer.
    static constexpr double HistogramMinLufs = -70.0;
    static constexpr double HistogramMaxLufs = 5.0;
    static constexpr double HistogramBinLu = 0.1;
    static constexpr size_t NumHistogramBins = 751;
    std::array<uint32_t, NumHistogramBins> m_gateCounts {};
    std::array<double, NumHistogramBins> m_gatePowerSums {};
    //! Running totals over every block past the absolute gate, which set the relative threshold.
    double m_absGatedPowerSum { 0.0 };
    uint64_t m_absGatedCount { 0 };

    // Read from the UI thread while the audio thread writes them.
    std::atomic<float> m_momentaryLufs { -70.0f };
    std::atomic<float> m_shortTermLufs { -70.0f };
    std::atomic<float> m_integratedLufs { -70.0f };
    std::atomic<bool> m_resetRequested { false };

    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // LUFS_METER_HPP
