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

#include "room_tail.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

using namespace RoomAcoustics;

//! The lines at a scale of one, in milliseconds. No two share a factor that would line their
//! echoes up; the room's mean free path scales all of them together.
constexpr std::array<double, RoomTail::LineCount> lineLengthsMs { 23.1, 26.9, 31.3, 36.7, 41.9, 47.3, 53.1, 59.9 };
constexpr double minLineScale { 0.35 };
constexpr double maxLineScale { 1.5 };
//! The mean line runs this many mean free paths: about as many reflections as a sound meets
//! before the room has mixed it.
constexpr double meanFreePathsPerLine { 3.0 };

constexpr double maxPreDelaySeconds { ImpulseDesigner::MaxWindowSeconds + 0.05 };

//! Where the shelves hand the middle band over to the low and the high one.
constexpr double lowShelfHz { 350.0 };
constexpr double highShelfHz { 3000.0 };

//! Which way each line takes its input, and which lines make up each output. The output rows are
//! orthogonal, so the left and the right of the diffuse field do not correlate.
constexpr std::array<double, RoomTail::LineCount> inputSigns { 1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0 };
constexpr std::array<double, RoomTail::LineCount> leftSigns { 1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0 };
constexpr std::array<double, RoomTail::LineCount> rightSigns { 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0 };

//! Below this the tail counts as gone: -120 dB.
constexpr double silentLevel { 1.0e-6 };

double flushDenormal(double value)
{
    return std::abs(value) < 1.0e-20 ? 0.0 : value;
}

//! The decay per band group, low (125 and 250 Hz), mid (500 Hz to 2 kHz) and high (4 and 8 kHz).
std::array<double, 3> bandGroupTimes(const Bands & times)
{
    return { std::sqrt(times[0] * times[1]), midReverbTime(times), std::sqrt(times[5] * times[6]) };
}

} // namespace

void RoomTail::configure(double sampleRate)
{
    m_sampleRate = sampleRate;
    const auto lineCapacity = static_cast<size_t>(std::ceil(lineLengthsMs.back() * maxLineScale * 0.001 * sampleRate)) + 2;
    for (auto && line : m_lines) {
        line.buffer.assign(lineCapacity, 0.0);
        line.length = 1;
    }
    const auto preDelayCapacity = static_cast<size_t>(std::ceil(maxPreDelaySeconds * sampleRate)) + 2;
    for (auto && input : m_inputs) {
        input.buffer.assign(preDelayCapacity, 0.0);
        input.low.calculate(lowShelfHz, sampleRate);
        input.high.calculate(highShelfHz, sampleRate);
    }
    reset();
}

bool RoomTail::isConfigured() const
{
    return m_sampleRate > 0.0;
}

void RoomTail::reset()
{
    for (auto && line : m_lines) {
        std::ranges::fill(line.buffer, 0.0);
        line.writePos = 0;
        line.low.reset();
        line.high.reset();
    }
    for (auto && input : m_inputs) {
        std::ranges::fill(input.buffer, 0.0);
        input.writePos = 0;
        input.low.reset();
        input.high.reset();
    }
    m_peak = 0.0;
    m_silentFrames = static_cast<size_t>(-1) / 2;
}

void RoomTail::set(const RoomSettings & settings, const Bands & reverbTimes, double startSeconds)
{
    if (!isConfigured()) {
        return;
    }
    const double v = volume(settings);
    const double meanFreePathSeconds = 4.0 * v / surfaceArea(settings) / SpeedOfSound;
    double meanLengthMs = 0.0;
    for (const double ms : lineLengthsMs) {
        meanLengthMs += ms / static_cast<double>(LineCount);
    }
    const double scale = std::clamp(meanFreePathsPerLine * meanFreePathSeconds * 1000.0 / meanLengthMs, minLineScale, maxLineScale);

    const auto times = bandGroupTimes(reverbTimes);
    double meanLength = 0.0;
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        line.length = std::clamp<size_t>(static_cast<size_t>(std::lround(lineLengthsMs[i] * scale * 0.001 * m_sampleRate)), 1, line.buffer.size() - 1);
        meanLength += static_cast<double>(line.length) / static_cast<double>(LineCount);
        // Sixty decibels down in the band's time, whatever the line's length
        const auto lineGain = [this, &line](double seconds) {
            return std::pow(10.0, -3.0 * static_cast<double>(line.length) / (seconds * m_sampleRate));
        };
        line.gain = lineGain(times[1]);
        line.lowShelf = lineGain(times[0]) / line.gain;
        line.highShelf = lineGain(times[2]) / line.gain;
        line.low.calculate(lowShelfHz, m_sampleRate);
        line.high.calculate(highShelfHz, m_sampleRate);
    }

    // In from where the first line would hand the tail over to the start
    const double shortest = static_cast<double>(m_lines.front().length) / m_sampleRate;
    const double preDelaySeconds = std::clamp(startSeconds - shortest, 0.0, maxPreDelaySeconds);
    m_preDelay = std::min(static_cast<size_t>(std::lround(preDelaySeconds * m_sampleRate)), m_inputs[0].buffer.size() - 1);

    // A unit impulse into four lines stores 4 G^2, which comes out at 4 G^2 / L a sample. That is
    // to be the room's diffuse energy at the pre-delay, per band.
    const double rate = diffuseEnergyRate(0.0, 1.0, v) / m_sampleRate;
    m_inputGain = std::sqrt(rate * meanLength / 4.0);
    const auto lost = [preDelaySeconds](double seconds) {
        return std::exp(-0.5 * 13.815510557964274 * preDelaySeconds / seconds);
    };
    m_inputGain *= lost(times[1]);
    m_inputLowShelf = lost(times[0]) / lost(times[1]);
    m_inputHighShelf = lost(times[2]) / lost(times[1]);
}

void RoomTail::process(double left, double right, double & outLeft, double & outRight)
{
    // The sources in, delayed to where the tail starts and tilted by what the room took by then
    std::array<double, 2> in { left, right };
    for (size_t i = 0; i < 2; i++) {
        auto && input = m_inputs[i];
        const size_t size = input.buffer.size();
        input.buffer[input.writePos] = in[i];
        double delayed = input.buffer[(input.writePos + size - m_preDelay) % size];
        input.writePos = (input.writePos + 1) % size;
        input.low.process(delayed);
        delayed += (m_inputLowShelf - 1.0) * input.low.lowPass();
        input.high.process(delayed);
        delayed += (m_inputHighShelf - 1.0) * input.high.highPass();
        in[i] = delayed * m_inputGain;
    }

    std::array<double, LineCount> outs {};
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        const size_t size = line.buffer.size();
        double out = line.buffer[(line.writePos + size - line.length) % size] * line.gain;
        line.low.process(out);
        out += (line.lowShelf - 1.0) * line.low.lowPass();
        line.high.process(out);
        out += (line.highShelf - 1.0) * line.high.highPass();
        outs[i] = flushDenormal(out);
    }

    // Hadamard, in place: three rounds of butterflies, then scaled to keep the energy
    std::array<double, LineCount> mixed = outs;
    for (size_t stride = 1; stride < LineCount; stride *= 2) {
        for (size_t i = 0; i < LineCount; i += stride * 2) {
            for (size_t j = i; j < i + stride; j++) {
                const double a = mixed[j];
                const double b = mixed[j + stride];
                mixed[j] = a + b;
                mixed[j + stride] = a - b;
            }
        }
    }
    const double norm = 1.0 / std::sqrt(static_cast<double>(LineCount));
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        // The left source into the even lines, the right into the odd ones
        line.buffer[line.writePos] = mixed[i] * norm + in[i % 2] * inputSigns[i];
        line.writePos = (line.writePos + 1) % line.buffer.size();
    }

    outLeft = 0.0;
    outRight = 0.0;
    for (size_t i = 0; i < LineCount; i++) {
        outLeft += outs[i] * leftSigns[i];
        outRight += outs[i] * rightSigns[i];
    }
    m_peak = std::max({ m_peak * 0.9995, std::abs(outLeft), std::abs(outRight) });
    m_silentFrames = std::max(std::abs(left), std::abs(right)) > silentLevel ? 0 : m_silentFrames + 1;
}

bool RoomTail::isRinging() const
{
    return m_peak > silentLevel || m_silentFrames <= m_preDelay + m_lines.back().length;
}

} // namespace noteahead
