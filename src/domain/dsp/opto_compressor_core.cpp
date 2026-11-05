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

#include "opto_compressor_core.hpp"

#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Where the side chain starts to pull down, after Peak Reduction's gain.
constexpr double thresholdDb { -20.0 };
//! Peak Reduction's side chain gain across its travel. Devices play at around -26 dBFS, where the
//! middle of the travel holds down 6 to 8 dB.
constexpr double minSideChainGainDb { -12.0 };
constexpr double maxSideChainGainDb { 48.0 };

constexpr double compressRatio { 3.0 };
constexpr double compressKneeDb { 10.0 };
constexpr double limitRatio { 10.0 };
constexpr double limitKneeDb { 6.0 };

//! The lamp lights at once and dims over a few cycles of the bass.
constexpr double lampReleaseSeconds { 0.010 };
//! The cell's first stage: about 10 ms to pull down, and half of the reduction gone in about
//! 60 ms when it lets go.
constexpr double fastAttackSeconds { 0.010 };
constexpr double fastReleaseSeconds { 0.040 };
//! The second stage charges while the cell is lit and holds the reduction it has stored for
//! seconds. Its share of the reduction is what the first stage cannot let go of.
constexpr double slowChargeSeconds { 1.5 };
constexpr double slowReleaseSeconds { 2.5 };
constexpr double slowShare { 0.4 };

constexpr double settledDb { 0.001 };
constexpr double silenceDb { -200.0 };

double coefficient(double seconds, double sampleRate)
{
    return 1.0 - std::exp(-1.0 / (seconds * sampleRate));
}

} // namespace

void OptoCompressorCore::setPeakReduction(double position)
{
    m_peakReduction = std::clamp(position, 0.0, 1.0);
}

void OptoCompressorCore::setLimit(bool limit)
{
    m_limit = limit;
}

void OptoCompressorCore::updateCoefficients()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    if (rate == m_lastSampleRate) {
        return;
    }
    m_lampRelease = coefficient(lampReleaseSeconds, rate);
    m_fastAttack = coefficient(fastAttackSeconds, rate);
    m_fastRelease = coefficient(fastReleaseSeconds, rate);
    m_slowCharge = coefficient(slowChargeSeconds, rate);
    m_slowRelease = coefficient(slowReleaseSeconds, rate);
    m_lastSampleRate = rate;
}

double OptoCompressorCore::targetReductionDb(double levelDb) const
{
    if (m_peakReduction <= 0.0) {
        return 0.0;
    }
    const double ratio = m_limit ? limitRatio : compressRatio;
    const double knee = m_limit ? limitKneeDb : compressKneeDb;
    const double sideChainDb = levelDb + minSideChainGainDb + m_peakReduction * (maxSideChainGainDb - minSideChainGainDb);
    const double over = sideChainDb - thresholdDb;
    const double slope = 1.0 - 1.0 / ratio;
    if (over <= -knee * 0.5) {
        return 0.0;
    }
    if (over < knee * 0.5) {
        const double into = over + knee * 0.5;
        return slope * into * into / (2.0 * knee);
    }
    return slope * over;
}

double OptoCompressorCore::processGainDb(double left, double right)
{
    updateCoefficients();

    // Linked: one lamp for both channels, so the image does not move
    const double rectified = std::max(std::abs(left), std::abs(right));
    m_lamp = rectified > m_lamp ? rectified : m_lamp + (rectified - m_lamp) * m_lampRelease;
    const double levelDb = m_lamp > 0.0 ? 20.0 * std::log10(m_lamp) : silenceDb;
    const double target = targetReductionDb(levelDb);

    m_fast += (target - m_fast) * (target > m_fast ? m_fastAttack : m_fastRelease);
    m_slow += (target - m_slow) * (target > m_slow ? m_slowCharge : m_slowRelease);

    // While pulling down the first stage leads. Letting go, the second holds back its share of
    // what it has stored.
    const double reduction = std::max(m_fast, (1.0 - slowShare) * m_fast + slowShare * m_slow);
    m_reductionDb = reduction < 1.0e-9 ? 0.0 : -reduction;
    if (m_fast < 1.0e-12) {
        m_fast = 0.0;
    }
    if (m_slow < 1.0e-12) {
        m_slow = 0.0;
    }
    if (m_lamp < 1.0e-12) {
        m_lamp = 0.0;
    }
    return m_reductionDb;
}

double OptoCompressorCore::reductionDb() const
{
    return m_reductionDb;
}

bool OptoCompressorCore::isSettled() const
{
    return std::abs(m_reductionDb) < settledDb && m_slow < settledDb && m_fast < settledDb;
}

void OptoCompressorCore::reset()
{
    m_lamp = 0.0;
    m_fast = 0.0;
    m_slow = 0.0;
    m_reductionDb = 0.0;
}

} // namespace noteahead
