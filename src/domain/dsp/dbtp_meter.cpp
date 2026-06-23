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

#include "dbtp_meter.hpp"

#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

static constexpr float dbtpFloor = -70.0f;

DbTpMeter::DbTpMeter()
{
}

std::string DbTpMeter::typeIdString()
{
    return Constants::RackEffectType::dbtpMeter().toStdString();
}

std::string DbTpMeter::type() const
{
    return typeIdString();
}

std::string DbTpMeter::typeId() const
{
    return typeIdString();
}

void DbTpMeter::updateDecayParams()
{
    if (m_sampleRate <= 0) {
        return;
    }
    // Running bar: -20 dB/s release (PPM-style)
    m_decayPerSample = std::pow(10.0, -1.0 / m_sampleRate);
    // Peak hold: 2 seconds, then -60 dB/s decay
    m_holdSamples = static_cast<int>(2.0 * m_sampleRate);
    m_holdDecayPerSample = std::pow(10.0, -3.0 / m_sampleRate);
}

// Catmull-Rom / Hermite interpolation at position t ∈ (0, 1)
// between buf[1] and buf[2], using buf[0] and buf[3] as context.
double DbTpMeter::hermiteAt(const std::array<double, 4> & buf, double t) const
{
    const double p0 = buf[0], p1 = buf[1], p2 = buf[2], p3 = buf[3];
    const double c1 = 0.5 * (p2 - p0);
    const double c2 = p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3;
    const double c3 = -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3;
    return ((c3 * t + c2) * t + c1) * t + p1;
}

void DbTpMeter::processChannel(double sample, double & peak, double & peakHold,
                               int & holdCounter, std::array<double, 4> & buf)
{
    buf[0] = buf[1];
    buf[1] = buf[2];
    buf[2] = buf[3];
    buf[3] = sample;

    // 4 values: original sample at buf[1] + 3 inter-samples in [buf[1], buf[2]]
    const double v0 = std::abs(buf[1]);
    const double v1 = std::abs(hermiteAt(buf, 0.25));
    const double v2 = std::abs(hermiteAt(buf, 0.50));
    const double v3 = std::abs(hermiteAt(buf, 0.75));
    const double instPeak = std::max({ v0, v1, v2, v3 });

    // Running bar: instant attack, exponential release
    if (instPeak > peak) {
        peak = instPeak;
    } else {
        peak *= m_decayPerSample;
    }

    // Peak hold: update when new maximum, hold for m_holdSamples, then decay
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

float DbTpMeter::linearToDbtp(double linear)
{
    if (linear < 1.0e-10) {
        return dbtpFloor;
    }
    return static_cast<float>(std::max(static_cast<double>(dbtpFloor), 20.0 * std::log10(linear)));
}

void DbTpMeter::process(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    if (const auto sr = static_cast<uint32_t>(m_sampleRate); sr != m_lastSampleRate) {
        m_lastSampleRate = sr;
        updateDecayParams();
        reset();
    }

    processChannel(left, m_peakL, m_peakHoldL, m_holdCounterL, m_bufL);
    processChannel(right, m_peakR, m_peakHoldR, m_holdCounterR, m_bufR);

    m_truePeakL = linearToDbtp(m_peakL);
    m_truePeakR = linearToDbtp(m_peakR);
    m_truePeakHoldL = linearToDbtp(m_peakHoldL);
    m_truePeakHoldR = linearToDbtp(m_peakHoldR);
}

void DbTpMeter::reset()
{
    m_bufL.fill(0.0);
    m_bufR.fill(0.0);
    m_peakL = 0.0;
    m_peakR = 0.0;
    m_peakHoldL = 0.0;
    m_peakHoldR = 0.0;
    m_holdCounterL = 0;
    m_holdCounterR = 0;
    m_truePeakL = dbtpFloor;
    m_truePeakR = dbtpFloor;
    m_truePeakHoldL = dbtpFloor;
    m_truePeakHoldR = dbtpFloor;
}

void DbTpMeter::sync()
{
}

float DbTpMeter::truePeakL() const
{
    return m_truePeakL;
}

float DbTpMeter::truePeakR() const
{
    return m_truePeakR;
}

float DbTpMeter::truePeakHoldL() const
{
    return m_truePeakHoldL;
}

float DbTpMeter::truePeakHoldR() const
{
    return m_truePeakHoldR;
}

} // namespace noteahead
