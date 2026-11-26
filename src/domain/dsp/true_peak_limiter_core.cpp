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

#include "true_peak_limiter_core.hpp"

#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace noteahead {

namespace {

constexpr double GuardFactor { 0.99426007 }; // -0.05 dB guard on detector target so peaks never exceed ceiling
constexpr double TruePeakFloorDb { -70.0 };
constexpr double SilenceThreshold { 1.0e-9 };
constexpr size_t DriftResetInterval { 2048 };
constexpr double FastReleaseMs { 25.0 };
constexpr double SustainedAttackMs { 30.0 };
constexpr double SustainedReleaseMs { 250.0 };
constexpr double SettledGainThreshold { 0.9999 };
constexpr size_t InitialQuietFrames { std::numeric_limits<size_t>::max() / 4 };

using PolyphaseTable = std::array<std::array<double, TruePeakLimiterCore::FirTapsPerPhase>, TruePeakLimiterCore::OversamplePhases>;

//! Precomputed 4x 48-tap (12 taps per phase) Kaiser-windowed sinc polyphase interpolator, aligned so
//! phase 0 outputs exact history[DetectorDelayFrames].
const PolyphaseTable & polyphaseTaps()
{
    static const PolyphaseTable table = [] {
        PolyphaseTable result {};
        constexpr double beta { 5.5 };
        const double invI0Beta { 1.0 / std::cyl_bessel_i(0.0, beta) };

        for (size_t phase = 0; phase < TruePeakLimiterCore::OversamplePhases; phase++) {
            const double t { static_cast<double>(phase) * 0.25 };
            const double center { static_cast<double>(TruePeakLimiterCore::DetectorDelayFrames) - t };
            double sum { 0.0 };
            for (size_t j = 0; j < TruePeakLimiterCore::FirTapsPerPhase; j++) {
                const double d { center - static_cast<double>(j) };
                const double sinc { std::abs(d) < 1.0e-12 ? 1.0 : std::sin(std::numbers::pi * d) / (std::numbers::pi * d) };
                const double u { d / static_cast<double>(TruePeakLimiterCore::DetectorDelayFrames) };
                const double win { std::cyl_bessel_i(0.0, beta * std::sqrt(std::max(0.0, 1.0 - u * u))) * invI0Beta };
                const double tap { sinc * win };
                result[phase][j] = tap;
                sum += tap;
            }
            const double invSum { sum != 0.0 ? 1.0 / sum : 1.0 };
            for (size_t j = 0; j < TruePeakLimiterCore::FirTapsPerPhase; j++) {
                result[phase][j] *= invSum;
            }
        }
        return result;
    }();
    return table;
}

float linearToTruePeakDb(double linear)
{
    if (linear < 1.0e-10) {
        return static_cast<float>(TruePeakFloorDb);
    }
    return static_cast<float>(std::max(TruePeakFloorDb, 20.0 * std::log10(linear)));
}

} // namespace

TruePeakLimiterCore::TruePeakLimiterCore()
{
    updateLevels();
    updateConfiguration(true);
    updateCoefficients();
    m_quietFrames = InitialQuietFrames;
}

void TruePeakLimiterCore::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    updateConfiguration(false);
    updateCoefficients();
}

void TruePeakLimiterCore::setThresholdDb(double db)
{
    m_thresholdDb = std::clamp(db, -24.0, 0.0);
    updateLevels();
}

void TruePeakLimiterCore::setCeilingDb(double db)
{
    m_ceilingDb = std::clamp(db, -6.0, 0.0);
    updateLevels();
}

void TruePeakLimiterCore::setReleaseMs(double ms)
{
    m_releaseMs = std::clamp(ms, 1.0, 1000.0);
    updateCoefficients();
}

void TruePeakLimiterCore::setLookaheadMs(double ms)
{
    m_lookaheadMs = std::clamp(ms, 0.0, 10.0);
    updateConfiguration(false);
}

void TruePeakLimiterCore::setBoost(bool boost)
{
    m_boost = boost;
    updateLevels();
}

void TruePeakLimiterCore::setAutoRelease(bool enabled)
{
    m_autoRelease = enabled;
}

void TruePeakLimiterCore::setBpm(double bpm)
{
    m_bpm = std::clamp(bpm, 40.0, 300.0);
    updateCoefficients();
}

double TruePeakLimiterCore::thresholdDb() const
{
    return m_thresholdDb;
}

double TruePeakLimiterCore::ceilingDb() const
{
    return m_ceilingDb;
}

double TruePeakLimiterCore::releaseMs() const
{
    return m_releaseMs;
}

double TruePeakLimiterCore::lookaheadMs() const
{
    return m_lookaheadMs;
}

bool TruePeakLimiterCore::boost() const
{
    return m_boost;
}

bool TruePeakLimiterCore::autoRelease() const
{
    return m_autoRelease;
}

double TruePeakLimiterCore::bpm() const
{
    return m_bpm;
}

size_t TruePeakLimiterCore::latencyFramesFor(double sampleRate, double lookaheadMs)
{
    const double sr { sampleRate > 0.0 ? sampleRate : 48000.0 };
    const double clampedLookahead { std::clamp(lookaheadMs, 0.0, 10.0) };
    const size_t lookaheadSamples { std::max<size_t>(1, static_cast<size_t>(std::lround(clampedLookahead * sr / 1000.0))) };
    return DetectorDelayFrames + (lookaheadSamples - 1);
}

size_t TruePeakLimiterCore::latencyFrames(double sampleRate) const
{
    return latencyFramesFor(sampleRate, m_lookaheadMs);
}

bool TruePeakLimiterCore::isSettled() const
{
    return m_meterEnvGain.load(std::memory_order_relaxed) >= SettledGainThreshold && m_quietFrames >= m_audioDelayFrames;
}

void TruePeakLimiterCore::updateLevels()
{
    m_thresholdLin = std::max(1.0e-6, static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(m_thresholdDb))));
    m_ceilingLin = std::max(1.0e-6, static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(m_ceilingDb))));
    m_targetLevel = m_boost ? m_thresholdLin : std::min(m_thresholdLin, m_ceilingLin);
    m_guardTarget = m_targetLevel * GuardFactor;
    m_postGain = m_boost ? (m_ceilingLin / m_thresholdLin) : 1.0;
}

void TruePeakLimiterCore::updateConfiguration(bool forceBufferReset)
{
    const double effectiveSr { m_sampleRate > 0.0 ? m_sampleRate : 48000.0 };
    if (!forceBufferReset && effectiveSr == m_configuredSampleRate && m_lookaheadMs == m_configuredLookaheadMs && !m_audioDelayL.empty()) {
        return;
    }

    m_lookaheadSamples = std::max<size_t>(1, static_cast<size_t>(std::lround(m_lookaheadMs * effectiveSr / 1000.0)));
    m_boxcar1Len = (m_lookaheadSamples + 1) / 2;
    m_boxcar2Len = m_lookaheadSamples + 1 - m_boxcar1Len;
    m_audioDelayFrames = DetectorDelayFrames + (m_lookaheadSamples - 1);

    const size_t audioRingSize { m_audioDelayFrames + 1 };
    m_audioDelayL.assign(audioRingSize, 0.0);
    m_audioDelayR.assign(audioRingSize, 0.0);
    m_audioWritePos = 0;

    m_winMinBuf.assign(m_lookaheadSamples, 1.0);
    m_minDeque.assign(m_lookaheadSamples, 0);
    dequeClear();
    m_winWritePos = 0;

    m_ma1Buf.assign(m_boxcar1Len, 1.0);
    m_ma1Idx = 0;
    m_ma1Sum = static_cast<double>(m_boxcar1Len);

    m_ma2Buf.assign(m_boxcar2Len, 1.0);
    m_ma2Idx = 0;
    m_ma2Sum = static_cast<double>(m_boxcar2Len);
    m_maDriftCounter = 0;

    m_envGain = 1.0;
    m_fastEnv = 1.0;
    m_slowEnv = 1.0;
    m_sustainedGr = 0.0;

    m_configuredSampleRate = effectiveSr;
    m_configuredLookaheadMs = m_lookaheadMs;
}

void TruePeakLimiterCore::updateCoefficients()
{
    const double effectiveSr { m_sampleRate > 0.0 ? m_sampleRate : 48000.0 };
    if (effectiveSr == m_coeffSampleRate && m_releaseMs == m_coeffReleaseMs && m_bpm == m_coeffBpm) {
        return;
    }

    m_manualReleaseCoeff = std::exp(-1.0 / (m_releaseMs * effectiveSr / 1000.0));
    m_fastReleaseCoeff = std::exp(-1.0 / (FastReleaseMs * effectiveSr / 1000.0));

    const double slowMs { std::clamp(0.5 * (60000.0 / m_bpm), 120.0, 350.0) };
    m_slowReleaseCoeff = std::exp(-1.0 / (slowMs * effectiveSr / 1000.0));
    m_sustainedAttackCoeff = std::exp(-1.0 / (SustainedAttackMs * effectiveSr / 1000.0));
    m_sustainedReleaseCoeff = std::exp(-1.0 / (SustainedReleaseMs * effectiveSr / 1000.0));

    m_peakDecayPerSample = std::pow(10.0, -1.0 / effectiveSr);
    m_holdSamples = static_cast<int>(2.0 * effectiveSr);
    m_holdDecayPerSample = std::pow(10.0, -3.0 / effectiveSr);

    m_coeffSampleRate = effectiveSr;
    m_coeffReleaseMs = m_releaseMs;
    m_coeffBpm = m_bpm;
}

double TruePeakLimiterCore::detectChannelTruePeak(double sample, std::array<double, FirTapsPerPhase * 2> & history, size_t & position) const
{
    position = position == 0 ? FirTapsPerPhase - 1 : position - 1;
    history[position] = sample;
    history[position + FirTapsPerPhase] = sample;

    const double * const h { history.data() + position };
    double peak { std::abs(h[DetectorDelayFrames]) };

    // 1. 4x BS.1770-4 Kaiser-windowed sinc polyphase FIR (phases 1..3; phase 0 == h[DetectorDelayFrames])
    const auto & taps { polyphaseTaps() };
    for (size_t phase = 1; phase < OversamplePhases; phase++) {
        double sum { 0.0 };
        const auto & phaseTaps { taps[phase] };
        for (size_t j = 0; j < FirTapsPerPhase; j++) {
            sum += phaseTaps[j] * h[j];
        }
        peak = std::max(peak, std::abs(sum));
    }

    // 2. 4x Catmull-Rom / Hermite interpolation matching DbTpMeter between h[6] and h[5]
    const double p0 { h[DetectorDelayFrames + 1] };
    const double p1 { h[DetectorDelayFrames] };
    const double p2 { h[DetectorDelayFrames - 1] };
    const double p3 { h[DetectorDelayFrames - 2] };
    const double c1 { 0.5 * (p2 - p0) };
    const double c2 { p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3 };
    const double c3 { -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3 };
    for (const double t : { 0.25, 0.50, 0.75 }) {
        const double v { ((c3 * t + c2) * t + c1) * t + p1 };
        peak = std::max(peak, std::abs(v));
    }

    return peak;
}

void TruePeakLimiterCore::trackOutputTruePeak(double sample, double & peak, double & peakHold, int & holdCounter, std::array<double, 4> & buf) const
{
    buf[0] = buf[1];
    buf[1] = buf[2];
    buf[2] = buf[3];
    buf[3] = sample;

    const double p0 { buf[0] };
    const double p1 { buf[1] };
    const double p2 { buf[2] };
    const double p3 { buf[3] };
    const double c1 { 0.5 * (p2 - p0) };
    const double c2 { p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3 };
    const double c3 { -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3 };
    const auto evalAt = [p1, c1, c2, c3](double t) {
        return ((c3 * t + c2) * t + c1) * t + p1;
    };

    const double v0 { std::abs(p1) };
    const double v1 { std::abs(evalAt(0.25)) };
    const double v2 { std::abs(evalAt(0.50)) };
    const double v3 { std::abs(evalAt(0.75)) };
    const double instPeak { std::max({ v0, v1, v2, v3 }) };

    if (instPeak > peak) {
        peak = instPeak;
    } else {
        peak *= m_peakDecayPerSample;
    }

    if (instPeak > peakHold) {
        peakHold = instPeak;
        holdCounter = m_holdSamples;
    } else if (holdCounter > 0) {
        holdCounter--;
    } else {
        peakHold *= m_holdDecayPerSample;
        if (peakHold < 1.0e-12) {
            peakHold = 0.0;
        }
    }
}

void TruePeakLimiterCore::processSingleFrame(double & left, double & right)
{
    if (m_resetPeakHoldRequested.exchange(false, std::memory_order_relaxed)) {
        m_outPeakHoldL = 0.0;
        m_outPeakHoldR = 0.0;
        m_outHoldCounterL = 0;
        m_outHoldCounterR = 0;
    }

    if (std::max(std::abs(left), std::abs(right)) < SilenceThreshold) {
        if (m_quietFrames < InitialQuietFrames) {
            m_quietFrames++;
        }
    } else {
        m_quietFrames = 0;
    }

    const double peakL { detectChannelTruePeak(left, m_detHistL, m_detHistPosL) };
    const double peakR { detectChannelTruePeak(right, m_detHistR, m_detHistPosR) };
    const double instTruePeak { std::max(peakL, peakR) };
    const double reqGain { instTruePeak > m_guardTarget ? (m_guardTarget / instTruePeak) : 1.0 };

    // Sliding-window minimum over L = m_lookaheadSamples frames via monotonic increasing deque
    if (!dequeEmpty() && dequeFront() == m_winWritePos) {
        dequePopFront();
    }
    while (!dequeEmpty() && m_winMinBuf[dequeBack()] >= reqGain) {
        dequePopBack();
    }
    m_winMinBuf[m_winWritePos] = reqGain;
    dequePushBack(m_winWritePos);
    m_winWritePos++;
    if (m_winWritePos >= static_cast<uint32_t>(m_lookaheadSamples)) {
        m_winWritePos = 0;
    }

    const double winMin { m_winMinBuf[dequeFront()] };

    // Release stage bounded strictly above by winMin
    double released { winMin };
    if (!m_autoRelease) {
        if (winMin < m_envGain) {
            m_envGain = winMin;
        } else {
            m_envGain = m_manualReleaseCoeff * m_envGain + (1.0 - m_manualReleaseCoeff) * winMin;
        }
        m_envGain = std::min(1.0, m_envGain);
        released = std::min(winMin, m_envGain);
    } else {
        if (winMin < m_fastEnv) {
            m_fastEnv = winMin;
        } else {
            m_fastEnv = m_fastReleaseCoeff * m_fastEnv + (1.0 - m_fastReleaseCoeff) * winMin;
        }
        if (winMin < m_slowEnv) {
            m_slowEnv = winMin;
        } else {
            m_slowEnv = m_slowReleaseCoeff * m_slowEnv + (1.0 - m_slowReleaseCoeff) * winMin;
        }
        m_fastEnv = std::min(1.0, m_fastEnv);
        m_slowEnv = std::min(1.0, m_slowEnv);

        const double grAmount { std::max(0.0, 1.0 - winMin) };
        const double trackerCoeff { grAmount > m_sustainedGr ? m_sustainedAttackCoeff : m_sustainedReleaseCoeff };
        m_sustainedGr = trackerCoeff * m_sustainedGr + (1.0 - trackerCoeff) * grAmount;
        const double slowWeight { std::clamp(m_sustainedGr * 6.0, 0.25, 1.0) };
        const double blended { m_fastEnv * (1.0 - slowWeight) + m_slowEnv * slowWeight };
        m_envGain = std::min({ 1.0, winMin, blended });
        released = m_envGain;
    }

    // Two cascaded boxcar moving averages for C1 smooth S-curve attack aligned to (L1 - 1) + (L2 - 1) == L - 1
    m_ma1Sum += released - m_ma1Buf[m_ma1Idx];
    m_ma1Buf[m_ma1Idx] = released;
    m_ma1Idx++;
    if (m_ma1Idx >= m_boxcar1Len) {
        m_ma1Idx = 0;
    }
    const double ma1Out { std::min(1.0, m_ma1Sum / static_cast<double>(m_boxcar1Len)) };

    m_ma2Sum += ma1Out - m_ma2Buf[m_ma2Idx];
    m_ma2Buf[m_ma2Idx] = ma1Out;
    m_ma2Idx++;
    if (m_ma2Idx >= m_boxcar2Len) {
        m_ma2Idx = 0;
    }

    m_maDriftCounter++;
    if (m_maDriftCounter >= DriftResetInterval) {
        m_maDriftCounter = 0;
        m_ma1Sum = std::accumulate(m_ma1Buf.begin(), m_ma1Buf.end(), 0.0);
        m_ma2Sum = std::accumulate(m_ma2Buf.begin(), m_ma2Buf.end(), 0.0);
    }

    const double smoothedGain { std::clamp(m_ma2Sum / static_cast<double>(m_boxcar2Len), 0.0, 1.0) };

    // Delay input audio by exact DetectorDelayFrames + (L - 1) frames
    const uint32_t delaySize { static_cast<uint32_t>(m_audioDelayL.size()) };
    m_audioDelayL[m_audioWritePos] = left;
    m_audioDelayR[m_audioWritePos] = right;
    const uint32_t readPos { m_audioWritePos + 1 < delaySize ? m_audioWritePos + 1 : 0 };

    const double totalGain { smoothedGain * m_postGain };
    const double outL { std::clamp(m_audioDelayL[readPos] * totalGain, -m_ceilingLin, m_ceilingLin) };
    const double outR { std::clamp(m_audioDelayR[readPos] * totalGain, -m_ceilingLin, m_ceilingLin) };
    m_audioWritePos = readPos;

    left = outL;
    right = outR;

    trackOutputTruePeak(outL, m_outPeakL, m_outPeakHoldL, m_outHoldCounterL, m_outHistL);
    trackOutputTruePeak(outR, m_outPeakR, m_outPeakHoldR, m_outHoldCounterR, m_outHistR);
}

void TruePeakLimiterCore::publishMeters()
{
    m_meterEnvGain.store(std::min(m_envGain, 1.0), std::memory_order_relaxed);
    m_meterOutputPeak.store(std::max(m_outPeakL, m_outPeakR), std::memory_order_relaxed);
    m_meterOutputPeakHold.store(std::max(m_outPeakHoldL, m_outPeakHoldR), std::memory_order_relaxed);
}

void TruePeakLimiterCore::processSample(double & left, double & right)
{
    updateConfiguration(false);
    updateCoefficients();
    processSingleFrame(left, right);
    publishMeters();
}

void TruePeakLimiterCore::processBlock(std::span<double> interleavedStereo, uint32_t frameCount)
{
    updateConfiguration(false);
    updateCoefficients();

    for (uint32_t i = 0; i < frameCount; i++) {
        processSingleFrame(interleavedStereo[i * 2], interleavedStereo[i * 2 + 1]);
    }
    publishMeters();
}

void TruePeakLimiterCore::reset()
{
    std::fill(m_detHistL.begin(), m_detHistL.end(), 0.0);
    std::fill(m_detHistR.begin(), m_detHistR.end(), 0.0);
    m_detHistPosL = 0;
    m_detHistPosR = 0;

    std::fill(m_audioDelayL.begin(), m_audioDelayL.end(), 0.0);
    std::fill(m_audioDelayR.begin(), m_audioDelayR.end(), 0.0);
    m_audioWritePos = 0;

    std::fill(m_winMinBuf.begin(), m_winMinBuf.end(), 1.0);
    dequeClear();
    m_winWritePos = 0;

    std::fill(m_ma1Buf.begin(), m_ma1Buf.end(), 1.0);
    m_ma1Idx = 0;
    m_ma1Sum = static_cast<double>(m_boxcar1Len);

    std::fill(m_ma2Buf.begin(), m_ma2Buf.end(), 1.0);
    m_ma2Idx = 0;
    m_ma2Sum = static_cast<double>(m_boxcar2Len);
    m_maDriftCounter = 0;

    m_envGain = 1.0;
    m_fastEnv = 1.0;
    m_slowEnv = 1.0;
    m_sustainedGr = 0.0;

    m_outPeakL = 0.0;
    m_outPeakR = 0.0;
    m_outPeakHoldL = 0.0;
    m_outPeakHoldR = 0.0;
    m_outHoldCounterL = 0;
    m_outHoldCounterR = 0;
    std::fill(m_outHistL.begin(), m_outHistL.end(), 0.0);
    std::fill(m_outHistR.begin(), m_outHistR.end(), 0.0);

    m_quietFrames = InitialQuietFrames;

    m_meterEnvGain.store(1.0, std::memory_order_relaxed);
    m_meterOutputPeak.store(0.0, std::memory_order_relaxed);
    m_meterOutputPeakHold.store(0.0, std::memory_order_relaxed);
    m_resetPeakHoldRequested.store(false, std::memory_order_relaxed);
}

float TruePeakLimiterCore::reductionDb() const
{
    const double envGain { m_meterEnvGain.load(std::memory_order_relaxed) };
    if (envGain >= 1.0) {
        return 0.0f;
    }
    const double db { static_cast<double>(Utils::Dsp::linearToDb(static_cast<float>(envGain))) };
    return std::abs(db) < 1.0e-12 ? 0.0f : static_cast<float>(db);
}

float TruePeakLimiterCore::outputTruePeakDb() const
{
    return linearToTruePeakDb(m_meterOutputPeak.load(std::memory_order_relaxed));
}

float TruePeakLimiterCore::outputTruePeakHoldDb() const
{
    return linearToTruePeakDb(m_meterOutputPeakHold.load(std::memory_order_relaxed));
}

void TruePeakLimiterCore::resetPeakHold()
{
    m_resetPeakHoldRequested.store(true, std::memory_order_relaxed);
    m_meterOutputPeakHold.store(0.0, std::memory_order_relaxed);
}

void TruePeakLimiterCore::dequeClear()
{
    m_dequeHead = 0;
    m_dequeCount = 0;
}

bool TruePeakLimiterCore::dequeEmpty() const
{
    return m_dequeCount == 0;
}

uint32_t TruePeakLimiterCore::dequeFront() const
{
    return m_minDeque[m_dequeHead];
}

uint32_t TruePeakLimiterCore::dequeBack() const
{
    const uint32_t cap { static_cast<uint32_t>(m_minDeque.size()) };
    return m_minDeque[(m_dequeHead + m_dequeCount - 1) % cap];
}

void TruePeakLimiterCore::dequePopFront()
{
    const uint32_t cap { static_cast<uint32_t>(m_minDeque.size()) };
    m_dequeHead = (m_dequeHead + 1) % cap;
    m_dequeCount--;
}

void TruePeakLimiterCore::dequePopBack()
{
    m_dequeCount--;
}

void TruePeakLimiterCore::dequePushBack(uint32_t value)
{
    const uint32_t cap { static_cast<uint32_t>(m_minDeque.size()) };
    m_minDeque[(m_dequeHead + m_dequeCount) % cap] = value;
    m_dequeCount++;
}

} // namespace noteahead
