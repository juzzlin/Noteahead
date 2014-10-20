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

#include "level_meter.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

float toDecibels(float linear)
{
    if (linear <= 0.0f) {
        return LevelMeter::MinimumDb;
    }
    return std::max(LevelMeter::MinimumDb, 20.0f * std::log10(linear));
}

} // namespace

void LevelMeter::setActive(bool active)
{
    m_active.store(active);
    if (!active) {
        reset();
    }
}

bool LevelMeter::active() const
{
    return m_active.load();
}

void LevelMeter::write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate)
{
    if (!m_active.load() || !frameCount || !sampleRate) {
        return;
    }

    float bufferPeak = 0.0f;
    double squareSum = 0.0;
    const uint32_t sampleCount = frameCount * 2;
    for (uint32_t i = 0; i < sampleCount; i++) {
        const auto sample = static_cast<float>(interleavedStereo[i]);
        bufferPeak = std::max(bufferPeak, std::abs(sample));
        squareSum += static_cast<double>(sample) * sample;
    }
    const auto bufferMeanSquare = static_cast<float>(squareSum / sampleCount);

    const float seconds = static_cast<float>(frameCount) / static_cast<float>(sampleRate);

    const float fallback = std::pow(10.0f, -PeakFallbackDbPerSecond * seconds / 20.0f);
    m_peak.store(std::max(bufferPeak, m_peak.load() * fallback));

    const float smoothing = 1.0f - std::exp(-seconds / RmsWindowSeconds);
    const float meanSquare = m_meanSquare.load();
    m_meanSquare.store(meanSquare + smoothing * (bufferMeanSquare - meanSquare));
}

float LevelMeter::peakDb() const
{
    return toDecibels(m_peak.load());
}

float LevelMeter::rmsDb() const
{
    return toDecibels(std::sqrt(m_meanSquare.load()));
}

void LevelMeter::reset()
{
    m_peak.store(0.0f);
    m_meanSquare.store(0.0f);
}

} // namespace noteahead
