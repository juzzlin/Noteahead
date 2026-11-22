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

#include "fet_compressor_core.hpp"

#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double MinAttackSeconds { 0.00002 };
constexpr double MaxAttackSeconds { 0.0008 };
constexpr double MinReleaseSeconds { 0.05 };
constexpr double MaxReleaseSeconds { 1.1 };

//! Second stage charges over ~250 ms and holds back a share of the reduction so sustained passages
//! recover smoothly without distorting bass transients.
constexpr double SlowChargeSeconds { 0.25 };
constexpr double AllLagSeconds { 0.0006 };
constexpr double ButterworthQ { 0.7071067811865476 };
constexpr double SettledDb { 0.001 };
constexpr double SilenceDb { -200.0 };

double coefficient(double seconds, double sampleRate)
{
    return 1.0 - std::exp(-1.0 / (std::max(1.0e-7, seconds) * sampleRate));
}

//! Open-loop feedback gain computer: reduction requested from a post-FET over-threshold level.
double feedbackCurveDb(double overThresholdDb, double ratio, double kneeDb)
{
    const double slope = std::max(0.0, ratio - 1.0);
    if (overThresholdDb <= -kneeDb * 0.5) {
        return 0.0;
    }
    if (overThresholdDb < kneeDb * 0.5 && kneeDb > 0.0) {
        const double into = overThresholdDb + kneeDb * 0.5;
        return slope * into * into / (2.0 * kneeDb);
    }
    return slope * overThresholdDb;
}

//! Closed-form solution to g = (1 - alpha) * prev + alpha * feedbackCurveDb(levelDb - g - threshold, ratio, knee).
double solveFeedbackStep(double levelDb, double prev, double alpha, double ratio, double thresholdDb, double kneeDb)
{
    const double base = (1.0 - alpha) * prev;
    const double halfKnee = kneeDb * 0.5;
    if (levelDb - base - thresholdDb <= -halfKnee) {
        return base;
    }

    const double slope = std::max(0.0, ratio - 1.0);
    const double gAbove = (base + alpha * slope * (levelDb - thresholdDb)) / (1.0 + alpha * slope);
    if (levelDb - gAbove - thresholdDb >= halfKnee || kneeDb <= 0.0) {
        return std::max(0.0, gAbove);
    }

    // Inside the quadratic knee, s = levelDb - thresholdDb + halfKnee - g > 0 satisfies a*s^2 + s - d = 0.
    const double a = alpha * slope / (2.0 * kneeDb);
    const double d = std::max(0.0, levelDb - thresholdDb + halfKnee - base);
    const double s = (2.0 * d) / (1.0 + std::sqrt(1.0 + 4.0 * a * d));
    return std::max(0.0, (levelDb - thresholdDb + halfKnee) - s);
}

} // namespace

FetCompressorCore::RatioCurve FetCompressorCore::curveFor(Ratio ratio)
{
    switch (ratio) {
    case Ratio::FourToOne:
        return { 4.0, -26.0, 8.0 };
    case Ratio::EightToOne:
        return { 8.0, -24.0, 6.0 };
    case Ratio::TwelveToOne:
        return { 12.0, -22.0, 4.0 };
    case Ratio::TwentyToOne:
        return { 20.0, -20.0, 2.5 };
    case Ratio::All:
        return { 20.0, -26.0, 1.5 };
    }
    return { 4.0, -26.0, 8.0 };
}

void FetCompressorCore::setRatio(Ratio ratio)
{
    m_ratio = ratio;
}

void FetCompressorCore::setAttackSeconds(double seconds)
{
    m_attackSeconds = std::clamp(seconds, MinAttackSeconds, MaxAttackSeconds);
}

void FetCompressorCore::setReleaseSeconds(double seconds)
{
    m_releaseSeconds = std::clamp(seconds, MinReleaseSeconds, MaxReleaseSeconds);
}

void FetCompressorCore::setSideChainHpfHz(double hz)
{
    m_hpfHz = hz > 0.0 ? hz : 0.0;
}

void FetCompressorCore::setAttackScale(double scale)
{
    m_attackScale = std::clamp(scale, 0.25, 2.0);
}

void FetCompressorCore::updateCoefficients()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const bool rateChanged = rate != m_lastSampleRate;

    if (rateChanged || m_attackSeconds != m_lastAttackSeconds || m_attackScale != m_lastAttackScale) {
        // Scaled by 4 so closed-loop attack time matches the nominal setting at 4:1 while tighter
        // ratios pull down proportionally faster through the feedback loop.
        const double openLoopAttackSeconds = 4.0 * m_attackSeconds * m_attackScale;
        m_attackCoeff = coefficient(openLoopAttackSeconds, rate);
        m_lastAttackSeconds = m_attackSeconds;
        m_lastAttackScale = m_attackScale;
    }

    if (rateChanged || m_releaseSeconds != m_lastReleaseSeconds || m_ratio != m_lastRatio) {
        const double fastScale = m_ratio == Ratio::All ? 0.50 : 0.65;
        const double slowScale = m_ratio == Ratio::All ? 1.40 : 1.80;
        m_fastReleaseCoeff = coefficient(fastScale * m_releaseSeconds, rate);
        m_slowReleaseCoeff = coefficient(slowScale * m_releaseSeconds, rate);
        m_lastReleaseSeconds = m_releaseSeconds;
        m_lastRatio = m_ratio;
    }

    if (rateChanged) {
        m_slowChargeCoeff = coefficient(SlowChargeSeconds, rate);
        m_allLagCoeff = coefficient(AllLagSeconds, rate);
    }

    if (rateChanged || m_hpfHz != m_lastHpfHz) {
        if (m_hpfHz > 0.0) {
            m_hpfL.calculateLowCut(m_hpfHz, rate, ButterworthQ);
            m_hpfR.calculateLowCut(m_hpfHz, rate, ButterworthQ);
        } else {
            m_hpfL.setBypass();
            m_hpfR.setBypass();
            m_hpfL.reset();
            m_hpfR.reset();
        }
        m_lastHpfHz = m_hpfHz;
    }

    m_lastSampleRate = rate;
}

double FetCompressorCore::targetReductionDb(double levelDb) const
{
    const auto curve = curveFor(m_ratio);
    return solveFeedbackStep(levelDb, 0.0, 1.0, curve.ratio, curve.thresholdDb, curve.kneeDb);
}

double FetCompressorCore::processGainDb(double left, double right)
{
    updateCoefficients();

    const double scL = m_hpfL.process(left);
    const double scR = m_hpfR.process(right);
    const double rectified = std::max(std::abs(scL), std::abs(scR));
    const double levelDb = rectified > 0.0 ? 20.0 * std::log10(rectified) : SilenceDb;

    const auto curve = curveFor(m_ratio);
    const double openTarget = feedbackCurveDb(levelDb - m_fast - curve.thresholdDb, curve.ratio, curve.kneeDb);
    const double alpha = openTarget > m_fast ? m_attackCoeff : m_fastReleaseCoeff;
    m_fast = solveFeedbackStep(levelDb, m_fast, alpha, curve.ratio, curve.thresholdDb, curve.kneeDb);

    // All-Buttons-In introduces a short pre-clamp lag so the leading edge of a transient snaps
    // through before the 20:1 limiter clamps down, while releasing instantaneously alongside m_fast.
    if (m_ratio == Ratio::All) {
        if (m_fast > m_allLag) {
            m_allLag += (m_fast - m_allLag) * m_allLagCoeff;
        } else {
            m_allLag = m_fast;
        }
    } else {
        m_allLag = m_fast;
    }

    const double activeFast = m_ratio == Ratio::All ? m_allLag : m_fast;
    m_slow += (activeFast - m_slow) * (activeFast > m_slow ? m_slowChargeCoeff : m_slowReleaseCoeff);

    const double slowShare = m_ratio == Ratio::All ? 0.20 : 0.30;
    const double reduction = std::max(activeFast, (1.0 - slowShare) * activeFast + slowShare * m_slow);

    if (m_fast < 1.0e-12) {
        m_fast = 0.0;
    }
    if (m_slow < 1.0e-12) {
        m_slow = 0.0;
    }
    if (m_allLag < 1.0e-12) {
        m_allLag = 0.0;
    }

    m_reductionDb = reduction < 1.0e-9 ? 0.0 : -reduction;
    return m_reductionDb;
}

double FetCompressorCore::reductionDb() const
{
    return m_reductionDb;
}

bool FetCompressorCore::isSettled() const
{
    return std::abs(m_reductionDb) < SettledDb && m_fast < SettledDb && m_slow < SettledDb && m_allLag < SettledDb;
}

void FetCompressorCore::reset()
{
    m_fast = 0.0;
    m_slow = 0.0;
    m_allLag = 0.0;
    m_reductionDb = 0.0;
    m_hpfL.reset();
    m_hpfR.reset();
}

} // namespace noteahead
