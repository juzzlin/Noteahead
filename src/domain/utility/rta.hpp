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

#ifndef RTA_HPP
#define RTA_HPP

#include "../effects/effect.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace noteahead {

// FFT-based real-time spectrum analyzer, passthrough insert effect.
// Multi-rate: slow FFT (LF bands) + fast FFT (HF bands >= CrossoverFreq).
// Both FFT sizes scale with band count so CPU scales too:
//   32 bands  → slow 8192,  fast 2048
//   64 bands  → slow 16384, fast 4096
//   128 bands → slow 32768, fast 8192
class Rta : public Effect
{
public:
    Rta();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;

    void reset() override;
    void sync() override;

    void setAnalysisEnabled(bool enabled);

    std::vector<float> bandMagnitudesDb() const;
    std::vector<std::pair<float, float>> bandLogPositions() const;

private:
    // Maximum (128-band) FFT sizes — arrays are always this large.
    static constexpr int MaxSlowFftSize = 32768;
    static constexpr int MaxFastFftSize = MaxSlowFftSize / 4; // 8192

    static constexpr int SlowHopSize = 512; // fixed; fast hop derives from FFT size + rate mode
    static constexpr double FreqLo = 20.0;
    static constexpr double FreqHi = 20000.0;
    static constexpr double CrossoverFreq = 400.0;

    void syncParameters();
    void buildWindows();
    void buildBands();
    void runSlowAnalysis();
    void runFastAnalysis();

    std::array<double, MaxSlowFftSize> m_slowWindow;
    std::array<double, MaxSlowFftSize> m_slowInBuf;
    std::array<double, MaxSlowFftSize> m_slowFftRe;
    std::array<double, MaxSlowFftSize> m_slowFftIm;
    int m_slowHopFill = 0;

    std::array<double, MaxFastFftSize> m_fastWindow;
    std::array<double, MaxFastFftSize> m_fastInBuf;
    std::array<double, MaxFastFftSize> m_fastFftRe;
    std::array<double, MaxFastFftSize> m_fastFftIm;
    int m_fastHopFill = 0;

    // Active FFT sizes — updated by buildBands() from m_bandCountMode.
    int m_slowFftN = MaxSlowFftSize;
    int m_fastFftN = MaxFastFftSize;
    int m_slowSpecBins = MaxSlowFftSize / 2 + 1;
    int m_fastSpecBins = MaxFastFftSize / 2 + 1;
    int m_fastHopSize = MaxFastFftSize / 16; // Normal overlap; recomputed in syncParameters

    // Audio-thread only:
    std::vector<double> m_smoothedPow;
    std::vector<std::pair<int, int>> m_bandBins;
    std::vector<bool> m_bandFast;
    std::vector<std::pair<float, float>> m_bandLogX;

    // Shared between audio and UI thread (protected by m_bandMutex):
    mutable std::mutex m_bandMutex;
    std::vector<float> m_bandDb;
    std::vector<std::pair<float, float>> m_bandLogXPublic;

    double m_sampleRateCached = 44100.0;
    uint32_t m_lastSampleRate = 0;

    int m_bandCountMode = 0; // 0=32, 1=64, 2=128
    int m_dbRangeMode = 2; // 0=-30, 1=-45, 2=-60, 3=-80
    bool m_showPinkNoise = true;
    float m_pinkNoiseLevel = -18.0f;
    int m_speedMode = 1; // 0=Fast, 1=Normal, 2=Slow
    int m_fftRateMode = 1; // 0=Fast(32x overlap), 1=Normal(16x), 2=Slow(8x)

    std::atomic<bool> m_analysisEnabled { false };
    bool m_shouldSync = false;
};

} // namespace noteahead

#endif // RTA_HPP
