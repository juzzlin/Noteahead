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

#include "stereo_level_meter.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

float toDecibels(float linear)
{
    if (linear <= 0.0f) {
        return StereoLevelMeter::MinimumDb;
    }
    return std::max(StereoLevelMeter::MinimumDb, 20.0f * std::log10(linear));
}

} // namespace

void StereoLevelMeter::setActive(bool active)
{
    m_active.store(active);
    if (!active) {
        reset();
    }
}

bool StereoLevelMeter::active() const
{
    return m_active.load();
}

void StereoLevelMeter::fold(Channel & channel, float bufferPeak, float bufferMeanSquare, float seconds)
{
    const float fallback = std::pow(10.0f, -PeakFallbackDbPerSecond * seconds / 20.0f);
    channel.peak.store(std::max(bufferPeak, channel.peak.load() * fallback));

    const float smoothing = 1.0f - std::exp(-seconds / RmsWindowSeconds);
    const float meanSquare = channel.meanSquare.load();
    channel.meanSquare.store(meanSquare + smoothing * (bufferMeanSquare - meanSquare));
}

void StereoLevelMeter::write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate)
{
    write(interleavedStereo, frameCount, sampleRate, 2);
}

void StereoLevelMeter::write(const double * interleaved, uint32_t frameCount, uint32_t sampleRate, uint32_t channelCount)
{
    if (!m_active.load() || !interleaved || !frameCount || !sampleRate || !channelCount) {
        return;
    }

    float leftPeak = 0.0f;
    float rightPeak = 0.0f;
    double leftSquareSum = 0.0;
    double rightSquareSum = 0.0;

    // The right channel of a mono buffer is its only channel, so both sides read the same samples.
    const uint32_t rightOffset = channelCount > 1 ? 1 : 0;
    for (uint32_t frame = 0; frame < frameCount; frame++) {
        const auto base = frame * channelCount;
        const auto left = static_cast<float>(interleaved[base]);
        const auto right = static_cast<float>(interleaved[base + rightOffset]);
        leftPeak = std::max(leftPeak, std::abs(left));
        rightPeak = std::max(rightPeak, std::abs(right));
        leftSquareSum += static_cast<double>(left) * left;
        rightSquareSum += static_cast<double>(right) * right;
    }

    const float seconds = static_cast<float>(frameCount) / static_cast<float>(sampleRate);
    fold(m_left, leftPeak, static_cast<float>(leftSquareSum / frameCount), seconds);
    fold(m_right, rightPeak, static_cast<float>(rightSquareSum / frameCount), seconds);
}

float StereoLevelMeter::leftPeakDb() const
{
    return toDecibels(m_left.peak.load());
}

float StereoLevelMeter::leftRmsDb() const
{
    return toDecibels(std::sqrt(m_left.meanSquare.load()));
}

float StereoLevelMeter::rightPeakDb() const
{
    return toDecibels(m_right.peak.load());
}

float StereoLevelMeter::rightRmsDb() const
{
    return toDecibels(std::sqrt(m_right.meanSquare.load()));
}

void StereoLevelMeter::reset()
{
    m_left.peak.store(0.0f);
    m_left.meanSquare.store(0.0f);
    m_right.peak.store(0.0f);
    m_right.meanSquare.store(0.0f);
}

} // namespace noteahead
