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

#ifndef LOUDNESS_METER_HPP
#define LOUDNESS_METER_HPP

#include <array>
#include <atomic>
#include <cstdint>

namespace noteahead {

//! Real-time loudness measurement per ITU-R BS.1770-4: momentary, short-term and gated integrated.
//!
//! One of three loudness measurements, and the only one usable from the audio thread on a signal
//! nobody is storing. LoudnessAnalyzer measures a finished file offline and adds range and true
//! peak; LufsMeter is this class behind the Effect interface, for a rack slot the user patches in.
//! A device's own tap owns one directly.
//!
//! Levels in dBFS say nothing about how loud two different sources are against each other: a kick
//! that hits eight times a bar reads far below a pad of the same perceived loudness on peak and on
//! RMS alike. K-weighting and the gating are what make the two comparable, which is the whole point
//! of metering a device this way rather than with a LevelMeter.
class LoudnessMeter
{
public:
    //! Loudness reported when nothing has been measured, and the floor of every reading. Also the
    //! absolute gate of BS.1770-4, below which a block plays no part in the integrated measurement.
    static constexpr float MinimumLufs { -70.0f };

    //! Gates write() so a meter nobody is watching costs nothing. Switching it off resets.
    //!
    //! Deliberately not consulted by processSample(): an owner feeding this one frame at a time has
    //! already decided to measure by calling it at all, and an Effect in a rack has no dialog to be
    //! gated by.
    void setActive(bool active);
    bool active() const;

    //! Audio-thread: fold one interleaved stereo buffer into the measurement. No-op when inactive.
    void write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate);

    //! Audio-thread: fold one stereo frame in. Ungated; set the sample rate separately.
    void processSample(double left, double right);

    //! Rate the following samples were taken at. A change re-derives the filters and starts the
    //! measurement over, since neither the filter state nor the blocks mean anything across one.
    void setSampleRate(uint32_t sampleRate);

    //! UI-thread: the three readings, in LUFS, clamped at MinimumLufs.
    float momentaryLufs() const;
    float shortTermLufs() const;
    //! Gated integrated loudness over everything measured since the last reset.
    float integratedLufs() const;

    //! Audio-thread: drops everything and starts over.
    void reset();

    //! Any thread: asks the audio thread to reset at its next sample. The readings blank at once, so
    //! a stopped engine cannot leave the previous take's numbers on screen.
    void requestReset();

private:
    void updateCoefficients();
    double applyKWeightL(double x);
    double applyKWeightR(double x);
    //! processSample() without the reset and sample-rate checks, which write() makes per buffer.
    void accumulateSample(double left, double right);
    void applyPendingState();
    void advanceBlock(double meanPower);
    void accumulateGatingBlock(double meanPower);
    void updateIntegrated();
    static size_t histogramBin(double lufs);

    std::atomic<bool> m_active { false };

    double m_sampleRate { 0.0 };

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
    std::atomic<float> m_momentaryLufs { MinimumLufs };
    std::atomic<float> m_shortTermLufs { MinimumLufs };
    std::atomic<float> m_integratedLufs { MinimumLufs };
    std::atomic<bool> m_resetRequested { false };

    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // LOUDNESS_METER_HPP
