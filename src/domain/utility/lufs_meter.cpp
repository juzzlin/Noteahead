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

#include "lufs_meter.hpp"

#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace noteahead {

static constexpr float lufsMin = -70.0f;

namespace {

//! BS.1770-4 loudness of a mean power, unclamped, so the absolute gate can tell a block that merely
//! sits at the floor from one that is genuinely below it.
double powerToLufs(double power)
{
    return power > 0.0 ? -0.691 + 10.0 * std::log10(power) : -std::numeric_limits<double>::infinity();
}

//! The same, clamped to the floor and narrowed for display.
float powerToDisplayLufs(double power)
{
    return static_cast<float>(std::max(static_cast<double>(lufsMin), powerToLufs(power)));
}

} // namespace

LufsMeter::LufsMeter()
{
}

std::string LufsMeter::typeIdString()
{
    return Constants::RackEffectType::lufsMeter().toStdString();
}

std::string LufsMeter::type() const
{
    return typeIdString();
}

std::string LufsMeter::typeId() const
{
    return typeIdString();
}

void LufsMeter::updateCoefficients()
{
    const double fs = m_sampleRate;
    if (fs <= 0) {
        return;
    }

    // Stage 1: high-shelf pre-filter per ITU-R BS.1770-4 (f0 = 1681.97 Hz, G ≈ +4 dB)
    {
        constexpr double f0 = 1681.974450955533;
        constexpr double G = 3.999843853973347;
        constexpr double Q = 0.7071067811865476;
        const double K = std::tan(std::numbers::pi * f0 / fs);
        const double Vh = std::pow(10.0, G / 20.0);
        const double Vb = std::pow(Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        m_b0s1 = (Vh + Vb * K / Q + K * K) / a0;
        m_b1s1 = 2.0 * (K * K - Vh) / a0;
        m_b2s1 = (Vh - Vb * K / Q + K * K) / a0;
        m_a1s1 = 2.0 * (K * K - 1.0) / a0;
        m_a2s1 = (1.0 - K / Q + K * K) / a0;
    }

    // Stage 2: RLB high-pass per ITU-R BS.1770-4 (f0 = 38.14 Hz)
    {
        constexpr double f0 = 38.13547087602444;
        constexpr double Q = 0.5003270373238773;
        const double K = std::tan(std::numbers::pi * f0 / fs);
        const double a0 = 1.0 + K / Q + K * K;
        m_b0s2 = 1.0 / a0;
        m_b1s2 = -2.0 / a0;
        m_b2s2 = 1.0 / a0;
        m_a1s2 = 2.0 * (K * K - 1.0) / a0;
        m_a2s2 = (1.0 - K / Q + K * K) / a0;
    }

    m_blockSize = std::max(size_t { 1 }, static_cast<size_t>(fs * 0.1));
}

double LufsMeter::applyKWeightL(double x)
{
    const double y1 = m_b0s1 * x + m_z1s1L;
    m_z1s1L = m_b1s1 * x - m_a1s1 * y1 + m_z2s1L;
    m_z2s1L = m_b2s1 * x - m_a2s1 * y1;
    const double y2 = m_b0s2 * y1 + m_z1s2L;
    m_z1s2L = m_b1s2 * y1 - m_a1s2 * y2 + m_z2s2L;
    m_z2s2L = m_b2s2 * y1 - m_a2s2 * y2;
    return y2;
}

double LufsMeter::applyKWeightR(double x)
{
    const double y1 = m_b0s1 * x + m_z1s1R;
    m_z1s1R = m_b1s1 * x - m_a1s1 * y1 + m_z2s1R;
    m_z2s1R = m_b2s1 * x - m_a2s1 * y1;
    const double y2 = m_b0s2 * y1 + m_z1s2R;
    m_z1s2R = m_b1s2 * y1 - m_a1s2 * y2 + m_z2s2R;
    m_z2s2R = m_b2s2 * y1 - m_a2s2 * y2;
    return y2;
}

void LufsMeter::advanceBlock(double meanPower)
{
    m_blocks[m_blockWriteIdx] = meanPower;
    m_blockWriteIdx = (m_blockWriteIdx + 1) % NumBlocks;
    if (m_blocksValid < NumBlocks) {
        m_blocksValid++;
    }

    // Momentary: mean of last 4 blocks (400 ms)
    const size_t mBlocks = std::min(m_blocksValid, size_t { 4 });
    double mPower = 0.0;
    for (size_t i = 0; i < mBlocks; i++) {
        mPower += m_blocks[(m_blockWriteIdx + NumBlocks - 1 - i) % NumBlocks];
    }
    const double momentaryPower = mPower / static_cast<double>(mBlocks);
    m_momentaryLufs = powerToDisplayLufs(momentaryPower);

    // Short-term: mean of all valid blocks (up to 3 seconds)
    double sPower = 0.0;
    for (size_t i = 0; i < m_blocksValid; i++) {
        sPower += m_blocks[i];
    }
    m_shortTermLufs = powerToDisplayLufs(sPower / static_cast<double>(m_blocksValid));

    // A gating block is 400 ms of audio, so the integrated measurement only starts once four 100-ms
    // blocks exist. From then on every block yields one, which is the 75 % overlap BS.1770-4 asks for,
    // and it is the momentary power by definition — no need to sum the window twice.
    if (mBlocks == 4) {
        accumulateGatingBlock(momentaryPower);
        updateIntegrated();
    }
}

size_t LufsMeter::histogramBin(double lufs)
{
    if (!(lufs > HistogramMinLufs)) {
        return 0;
    }
    const auto bin = static_cast<size_t>((std::min(lufs, HistogramMaxLufs) - HistogramMinLufs) / HistogramBinLu);
    return std::min(bin, NumHistogramBins - 1);
}

void LufsMeter::accumulateGatingBlock(double meanPower)
{
    // Absolute gate: anything below -70 LUFS, digital silence included, plays no part in the
    // measurement at all — neither in the average nor in the relative threshold it sets.
    if (const auto loudness = powerToLufs(meanPower); loudness < HistogramMinLufs) {
        return;
    }
    m_absGatedPowerSum += meanPower;
    m_absGatedCount++;
    const auto bin = histogramBin(powerToLufs(meanPower));
    m_gateCounts[bin]++;
    m_gatePowerSums[bin] += meanPower;
}

void LufsMeter::updateIntegrated()
{
    if (!m_absGatedCount) {
        m_integratedLufs = lufsMin;
        return;
    }

    // Relative gate: 10 LU below the mean of everything that passed the absolute gate.
    const auto absGatedMean = m_absGatedPowerSum / static_cast<double>(m_absGatedCount);
    const auto relativeThreshold = powerToLufs(absGatedMean) - 10.0;

    double gatedSum = 0.0;
    uint64_t gatedCount = 0;
    for (auto bin = histogramBin(relativeThreshold); bin < NumHistogramBins; bin++) {
        gatedSum += m_gatePowerSums[bin];
        gatedCount += m_gateCounts[bin];
    }

    // Only the bin straddling the threshold is approximate; the mean itself comes from exact sums.
    m_integratedLufs = powerToDisplayLufs(gatedCount ? gatedSum / static_cast<double>(gatedCount) : absGatedMean);
}

void LufsMeter::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    if (m_resetRequested.exchange(false)) {
        reset();
    }

    if (const auto sr = static_cast<uint32_t>(m_sampleRate); sr != m_lastSampleRate) {
        m_lastSampleRate = sr;
        updateCoefficients();
        reset();
    }

    const double wL = applyKWeightL(left);
    const double wR = applyKWeightR(right);
    m_blockPowerSum += wL * wL + wR * wR;
    m_blockSamples++;

    if (m_blockSamples >= m_blockSize) {
        advanceBlock(m_blockPowerSum / static_cast<double>(m_blockSamples));
        m_blockPowerSum = 0.0;
        m_blockSamples = 0;
    }
}

void LufsMeter::reset()
{
    m_z1s1L = m_z2s1L = m_z1s2L = m_z2s2L = 0.0;
    m_z1s1R = m_z2s1R = m_z1s2R = m_z2s2R = 0.0;
    m_blockPowerSum = 0.0;
    m_blockSamples = 0;
    m_blocks.fill(0.0);
    m_blockWriteIdx = 0;
    m_blocksValid = 0;
    m_gateCounts.fill(0);
    m_gatePowerSums.fill(0.0);
    m_absGatedPowerSum = 0.0;
    m_absGatedCount = 0;
    m_momentaryLufs = lufsMin;
    m_shortTermLufs = lufsMin;
    m_integratedLufs = lufsMin;
}

void LufsMeter::requestReset()
{
    // Blank the readings up front so a stopped engine does not leave the previous take's numbers on
    // screen while it waits for a sample that clears the rest.
    m_momentaryLufs = lufsMin;
    m_shortTermLufs = lufsMin;
    m_integratedLufs = lufsMin;
    m_resetRequested = true;
}

void LufsMeter::sync()
{
}

float LufsMeter::momentaryLufs() const
{
    return m_momentaryLufs;
}

float LufsMeter::shortTermLufs() const
{
    return m_shortTermLufs;
}

float LufsMeter::integratedLufs() const
{
    return m_integratedLufs;
}

} // namespace noteahead
