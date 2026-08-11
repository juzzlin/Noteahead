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

#include "loudness_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace noteahead {

static constexpr float lufsFloor = -70.0f;

LoudnessAnalyzer::LoudnessAnalyzer(double sampleRate)
  : m_sampleRate { sampleRate }
{
    m_dbtpMeter.setSampleRate(m_sampleRate);
    updateCoefficients();
}

void LoudnessAnalyzer::updateCoefficients()
{
    const double fs = m_sampleRate;
    if (fs <= 0.0) {
        return;
    }

    // Stage 1: high-shelf pre-filter per ITU-R BS.1770-4
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

    // Stage 2: RLB high-pass per ITU-R BS.1770-4
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

double LoudnessAnalyzer::applyKWeightL(double x)
{
    const double y1 = m_b0s1 * x + m_z1s1L;
    m_z1s1L = m_b1s1 * x - m_a1s1 * y1 + m_z2s1L;
    m_z2s1L = m_b2s1 * x - m_a2s1 * y1;
    const double y2 = m_b0s2 * y1 + m_z1s2L;
    m_z1s2L = m_b1s2 * y1 - m_a1s2 * y2 + m_z2s2L;
    m_z2s2L = m_b2s2 * y1 - m_a2s2 * y2;
    return y2;
}

double LoudnessAnalyzer::applyKWeightR(double x)
{
    const double y1 = m_b0s1 * x + m_z1s1R;
    m_z1s1R = m_b1s1 * x - m_a1s1 * y1 + m_z2s1R;
    m_z2s1R = m_b2s1 * x - m_a2s1 * y1;
    const double y2 = m_b0s2 * y1 + m_z1s2R;
    m_z1s2R = m_b1s2 * y1 - m_a1s2 * y2 + m_z2s2R;
    m_z2s2R = m_b2s2 * y1 - m_a2s2 * y2;
    return y2;
}

void LoudnessAnalyzer::process(const float * data, size_t numSamples)
{
    for (size_t i = 0; i < numSamples; i += 2) {
        double left = data[i];
        double right = data[i + 1];

        // Process True Peak
        m_dbtpMeter.process(left, right);
        m_maxDbTp = std::max({ m_maxDbTp, m_dbtpMeter.truePeakL(), m_dbtpMeter.truePeakR() });

        // Apply K-weighting
        const double wL = applyKWeightL(left);
        const double wR = applyKWeightR(right);

        // Accumulate mean square power sum (stereo weights are both 1.0)
        m_blockPowerSum += (wL * wL + wR * wR);
        m_blockSamples++;

        if (m_blockSamples >= m_blockSize) {
            m_blockPowers.push_back(m_blockPowerSum / static_cast<double>(m_blockSamples));
            m_blockPowerSum = 0.0;
            m_blockSamples = 0;
        }
    }
}

LoudnessAnalyzer::Result LoudnessAnalyzer::calculate()
{
    Result result {};
    result.truePeak = m_maxDbTp;

    // If there is leftover audio that didn't fill the last 100 ms block, include it
    if (m_blockSamples > 0) {
        m_blockPowers.push_back(m_blockPowerSum / static_cast<double>(m_blockSamples));
    }

    if (m_blockPowers.empty()) {
        return result;
    }

    const auto toLoudness = [](double power) -> float {
        if (power < 1.0e-10) {
            return lufsFloor;
        }
        return static_cast<float>(std::max(static_cast<double>(lufsFloor), -0.691 + 10.0 * std::log10(power)));
    };

    // The same, unclamped. The absolute gate has to be able to tell a block resting at the floor from
    // one genuinely beneath it; asking toLoudness() would let everything through, silence included,
    // because it never returns less than the floor it is being compared against.
    const auto toUngatedLoudness = [](double power) -> double {
        return power > 0.0 ? -0.691 + 10.0 * std::log10(power) : -std::numeric_limits<double>::infinity();
    };

    // 1. Calculate Integrated Loudness (400 ms blocks, overlapping by 75%)
    // Each 400 ms block is composed of 4 consecutive 100 ms blocks.
    std::vector<double> blockPowers400ms {};
    if (m_blockPowers.size() >= 4) {
        for (size_t i = 3; i < m_blockPowers.size(); i++) {
            const double sum = m_blockPowers[i] + m_blockPowers[i - 1] + m_blockPowers[i - 2] + m_blockPowers[i - 3];
            blockPowers400ms.push_back(sum / 4.0);
        }
    } else {
        // Fallback for short audio: use the average of whatever blocks we have
        double sum = 0.0;
        for (double p : m_blockPowers) {
            sum += p;
        }
        blockPowers400ms.push_back(sum / static_cast<double>(m_blockPowers.size()));
    }

    // Absolute Gate (-70 LUFS)
    std::vector<double> absGatedPowers {};
    for (double p : blockPowers400ms) {
        if (toUngatedLoudness(p) >= lufsFloor) {
            absGatedPowers.push_back(p);
        }
    }

    if (!absGatedPowers.empty()) {
        // Compute average power of absolute-gated blocks
        double absSum = 0.0;
        for (double p : absGatedPowers) {
            absSum += p;
        }
        const double absAvgPower = absSum / static_cast<double>(absGatedPowers.size());

        // Relative Gate (-10 dB relative to absolute gated loudness)
        const float relativeThreshold = toLoudness(absAvgPower) - 10.0f;
        result.threshold = relativeThreshold;

        std::vector<double> relGatedPowers {};
        for (double p : absGatedPowers) {
            if (toLoudness(p) >= relativeThreshold) {
                relGatedPowers.push_back(p);
            }
        }

        if (!relGatedPowers.empty()) {
            double relSum = 0.0;
            for (double p : relGatedPowers) {
                relSum += p;
            }
            result.integratedLoudness = toLoudness(relSum / static_cast<double>(relGatedPowers.size()));
        } else {
            result.integratedLoudness = toLoudness(absAvgPower);
        }
    } else {
        result.integratedLoudness = lufsFloor;
        result.threshold = lufsFloor;
    }

    // 2. Calculate Loudness Range (LRA) (3-second blocks, overlapping by 2.9 seconds)
    // 3 seconds is exactly 30 consecutive 100 ms blocks.
    std::vector<double> blockPowersst {};
    if (m_blockPowers.size() >= 30) {
        for (size_t i = 29; i < m_blockPowers.size(); i++) {
            double sum = 0.0;
            for (size_t j = 0; j < 30; j++) {
                sum += m_blockPowers[i - j];
            }
            blockPowersst.push_back(sum / 30.0);
        }
    } else {
        // Fallback for short audio: use 3-second block of whatever we have
        double sum = 0.0;
        for (double p : m_blockPowers) {
            sum += p;
        }
        blockPowersst.push_back(sum / static_cast<double>(m_blockPowers.size()));
    }

    // Absolute Gate for LRA (-70 LUFS)
    std::vector<double> lraAbsPowers {};
    for (double p : blockPowersst) {
        if (toUngatedLoudness(p) >= lufsFloor) {
            lraAbsPowers.push_back(p);
        }
    }

    if (!lraAbsPowers.empty()) {
        double lraAbsSum = 0.0;
        for (double p : lraAbsPowers) {
            lraAbsSum += p;
        }
        const double lraAbsAvgPower = lraAbsSum / static_cast<double>(lraAbsPowers.size());

        // Relative Gate for LRA (-20 dB relative to absolute gated short-term loudness)
        const float lraRelThreshold = toLoudness(lraAbsAvgPower) - 20.0f;

        std::vector<float> stLoudnessGated {};
        for (double p : lraAbsPowers) {
            const float l = toLoudness(p);
            if (l >= lraRelThreshold) {
                stLoudnessGated.push_back(l);
            }
        }

        if (stLoudnessGated.size() >= 2) {
            std::sort(stLoudnessGated.begin(), stLoudnessGated.end());
            const size_t N = stLoudnessGated.size();
            const size_t idx10 = static_cast<size_t>(std::round(0.10 * static_cast<double>(N - 1)));
            const size_t idx95 = static_cast<size_t>(std::round(0.95 * static_cast<double>(N - 1)));
            result.loudnessRange = stLoudnessGated[idx95] - stLoudnessGated[idx10];
        } else {
            result.loudnessRange = 0.0f;
        }
    } else {
        result.loudnessRange = 0.0f;
    }

    return result;
}

} // namespace noteahead
