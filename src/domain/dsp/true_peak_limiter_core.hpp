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

#ifndef TRUE_PEAK_LIMITER_CORE_HPP
#define TRUE_PEAK_LIMITER_CORE_HPP

#include "dsp_component.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace noteahead {

//! Zero-allocation real-time-safe 4x true-peak lookahead brickwall limiter core.
class TruePeakLimiterCore : public DspComponent
{
public:
    static constexpr size_t DetectorDelayFrames { 6 };
    static constexpr size_t FirTapsPerPhase { 12 };
    static constexpr size_t OversamplePhases { 4 };

    TruePeakLimiterCore();

    void setSampleRate(double sampleRate) override;

    void setThresholdDb(double db);
    void setCeilingDb(double db);
    void setReleaseMs(double ms);
    void setLookaheadMs(double ms);
    void setBoost(bool boost);
    void setAutoRelease(bool enabled);
    void setBpm(double bpm);

    double thresholdDb() const;
    double ceilingDb() const;
    double releaseMs() const;
    double lookaheadMs() const;
    bool boost() const;
    bool autoRelease() const;
    double bpm() const;

    static size_t latencyFramesFor(double sampleRate, double lookaheadMs);
    size_t latencyFrames(double sampleRate) const;
    bool isSettled() const;

    void processSample(double & left, double & right);
    void processBlock(std::span<double> interleavedStereo, uint32_t frameCount);
    void reset();

    float reductionDb() const;
    float outputTruePeakDb() const;
    float outputTruePeakHoldDb() const;
    void resetPeakHold();

private:
    void updateLevels();
    void updateConfiguration(bool forceBufferReset);
    void updateCoefficients();
    void processSingleFrame(double & left, double & right);
    double detectChannelTruePeak(double sample, std::array<double, FirTapsPerPhase * 2> & history, size_t & position) const;
    void trackOutputTruePeak(double sample, double & peak, double & peakHold, int & holdCounter, std::array<double, 4> & buf) const;
    void publishMeters();

    void dequeClear();
    bool dequeEmpty() const;
    uint32_t dequeFront() const;
    uint32_t dequeBack() const;
    void dequePopFront();
    void dequePopBack();
    void dequePushBack(uint32_t value);

    double m_thresholdDb { 0.0 };
    double m_ceilingDb { -1.0 };
    double m_releaseMs { 100.0 };
    double m_lookaheadMs { 5.0 };
    bool m_boost { false };
    bool m_autoRelease { false };
    double m_bpm { 120.0 };

    double m_thresholdLin { 1.0 };
    double m_ceilingLin { 1.0 };
    double m_targetLevel { 1.0 };
    double m_guardTarget { 1.0 };
    double m_postGain { 1.0 };

    double m_configuredSampleRate { 0.0 };
    double m_configuredLookaheadMs { -1.0 };
    double m_coeffSampleRate { 0.0 };
    double m_coeffReleaseMs { -1.0 };
    double m_coeffBpm { -1.0 };

    double m_manualReleaseCoeff { 0.0 };
    double m_fastReleaseCoeff { 0.0 };
    double m_slowReleaseCoeff { 0.0 };
    double m_sustainedAttackCoeff { 0.0 };
    double m_sustainedReleaseCoeff { 0.0 };
    double m_peakDecayPerSample { 0.0 };
    double m_holdDecayPerSample { 0.0 };
    int m_holdSamples { 0 };

    size_t m_lookaheadSamples { 1 };
    size_t m_boxcar1Len { 1 };
    size_t m_boxcar2Len { 1 };
    size_t m_audioDelayFrames { DetectorDelayFrames };

    std::array<double, FirTapsPerPhase * 2> m_detHistL {};
    std::array<double, FirTapsPerPhase * 2> m_detHistR {};
    size_t m_detHistPosL { 0 };
    size_t m_detHistPosR { 0 };

    std::vector<double> m_audioDelayL;
    std::vector<double> m_audioDelayR;
    uint32_t m_audioWritePos { 0 };

    std::vector<double> m_winMinBuf;
    std::vector<uint32_t> m_minDeque;
    uint32_t m_winWritePos { 0 };
    uint32_t m_dequeHead { 0 };
    uint32_t m_dequeCount { 0 };

    double m_envGain { 1.0 };
    double m_fastEnv { 1.0 };
    double m_slowEnv { 1.0 };
    double m_sustainedGr { 0.0 };

    std::vector<double> m_ma1Buf;
    size_t m_ma1Idx { 0 };
    double m_ma1Sum { 1.0 };

    std::vector<double> m_ma2Buf;
    size_t m_ma2Idx { 0 };
    double m_ma2Sum { 1.0 };
    size_t m_maDriftCounter { 0 };

    double m_outPeakL { 0.0 };
    double m_outPeakR { 0.0 };
    double m_outPeakHoldL { 0.0 };
    double m_outPeakHoldR { 0.0 };
    int m_outHoldCounterL { 0 };
    int m_outHoldCounterR { 0 };
    std::array<double, 4> m_outHistL {};
    std::array<double, 4> m_outHistR {};

    size_t m_quietFrames { DetectorDelayFrames };

    std::atomic<double> m_meterEnvGain { 1.0 };
    std::atomic<double> m_meterOutputPeak { 0.0 };
    std::atomic<double> m_meterOutputPeakHold { 0.0 };
    std::atomic<bool> m_resetPeakHoldRequested { false };
};

} // namespace noteahead

#endif // TRUE_PEAK_LIMITER_CORE_HPP
