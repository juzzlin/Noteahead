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

#include "compressor_core.hpp"

#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Averaging time of the RMS detector. Short enough to still catch syllables, long enough to
//! ignore individual peaks the way an RMS-sensing compressor is expected to.
constexpr double rmsWindowMs = 10.0;

} // namespace

void CompressorCore::setThresholdDb(double thresholdDb)
{
    m_thresholdDb = thresholdDb;
}

void CompressorCore::setRatio(double ratio)
{
    m_ratio = ratio;
}

void CompressorCore::setKneeDb(double kneeDb)
{
    m_kneeDb = kneeDb;
}

void CompressorCore::setAttackMs(double attackMs)
{
    m_attackMs = attackMs;
}

void CompressorCore::setReleaseMs(double releaseMs)
{
    m_releaseMs = releaseMs;
}

void CompressorCore::setDetectorMode(DetectorMode mode)
{
    if (mode != m_detectorMode) {
        m_rmsSquare = 0.0;
    }
    m_detectorMode = mode;
}

void CompressorCore::updateCoefficients()
{
    if (m_sampleRate <= 0.0) {
        return;
    }

    if (std::abs(m_attackMs - m_lastAttackMs) < 1.0e-9 && std::abs(m_releaseMs - m_lastReleaseMs) < 1.0e-9 && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    m_attackCoeff = std::exp(-1.0 / (m_attackMs * m_sampleRate / 1000.0));
    m_releaseCoeff = std::exp(-1.0 / (m_releaseMs * m_sampleRate / 1000.0));
    m_rmsCoeff = std::exp(-1.0 / (rmsWindowMs * m_sampleRate / 1000.0));

    m_lastAttackMs = m_attackMs;
    m_lastReleaseMs = m_releaseMs;
    m_lastSampleRate = m_sampleRate;
}

double CompressorCore::detectorLevelDb(double left, double right)
{
    if (m_detectorMode == DetectorMode::Rms) {
        const double meanSquare = (left * left + right * right) * 0.5;
        m_rmsSquare = m_rmsCoeff * m_rmsSquare + (1.0 - m_rmsCoeff) * meanSquare;
        // Denormal protection
        if (m_rmsSquare < 1.0e-30) {
            m_rmsSquare = 0.0;
        }
        return Utils::Dsp::linearToDb(static_cast<float>(std::sqrt(m_rmsSquare)));
    }

    const double detector = std::max(std::abs(left), std::abs(right));
    return Utils::Dsp::linearToDb(static_cast<float>(detector));
}

double CompressorCore::gainComputerDb(double detectorDb) const
{
    double targetDb = detectorDb;

    if (m_kneeDb > 0.001) {
        if (detectorDb > m_thresholdDb + m_kneeDb / 2.0) {
            targetDb = m_thresholdDb + (detectorDb - m_thresholdDb) / m_ratio;
        } else if (detectorDb > m_thresholdDb - m_kneeDb / 2.0) {
            const double diff = detectorDb - m_thresholdDb + m_kneeDb / 2.0;
            targetDb = detectorDb + (1.0 / m_ratio - 1.0) * diff * diff / (2.0 * m_kneeDb);
        }
    } else {
        if (detectorDb > m_thresholdDb) {
            targetDb = m_thresholdDb + (detectorDb - m_thresholdDb) / m_ratio;
        }
    }

    return targetDb - detectorDb;
}

double CompressorCore::processGainDb(double left, double right)
{
    updateCoefficients();

    const double gainReductionDb = gainComputerDb(detectorLevelDb(left, right));

    if (gainReductionDb < m_envelopeDb) {
        m_envelopeDb = m_attackCoeff * m_envelopeDb + (1.0 - m_attackCoeff) * gainReductionDb;
    } else {
        m_envelopeDb = m_releaseCoeff * m_envelopeDb + (1.0 - m_releaseCoeff) * gainReductionDb;
    }

    // Denormal protection
    if (std::abs(m_envelopeDb) < 1.0e-15) {
        m_envelopeDb = 0.0;
    }

    return m_envelopeDb;
}

double CompressorCore::reductionDb() const
{
    return m_envelopeDb;
}

void CompressorCore::reset()
{
    m_envelopeDb = 0.0;
    m_rmsSquare = 0.0;
}

} // namespace noteahead
