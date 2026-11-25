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
#include <bit>
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

//! Input Schroeder all-pass diffuser lengths and alternating coefficients.
constexpr std::array<double, RoomTail::DiffuserCount> diffuserLengthsMs { 1.37, 2.33, 3.67, 5.23 };
constexpr std::array<double, RoomTail::DiffuserCount> diffuserCoeffs { 0.62, -0.58, 0.54, -0.50 };
constexpr double rightDiffuserStretch { 1.093 };

//! Subtle delay-line drift to break up static metallic FDN eigenmodes without pitch shift.
constexpr double modBaseRateHz { 0.37 };
constexpr double modRateStepHz { 0.043 };
constexpr double modDepthMs { 0.22 };
constexpr double parameterSmoothSeconds { 0.02 };

constexpr double maxPreDelaySeconds { ImpulseDesigner::MaxWindowSeconds + 0.05 };

//! Where the shelves hand the middle band over to the low and the high one.
constexpr double lowCutHz { 60.0 };
constexpr double lowShelfHz { 350.0 };
constexpr double highShelfHz { 3000.0 };

//! Which way each line takes its input, and how each line is panned into the stereo field. The
//! eight output angles are spaced evenly around the unit circle, so Left and Right are orthogonal,
//! have equal power on even and odd lines, and pan every line to its own stereo azimuth rather than
//! flipping between dead-centre mono and out-of-phase side.
constexpr std::array<double, RoomTail::LineCount> inputSigns { 1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0 };
constexpr std::array<double, RoomTail::LineCount> leftWeights {
    1.3065629648763766, 0.5411961001461970, -0.5411961001461970, -1.3065629648763766,
    -1.3065629648763766, -0.5411961001461970, 0.5411961001461970, 1.3065629648763766
};
constexpr std::array<double, RoomTail::LineCount> rightWeights {
    0.5411961001461970, 1.3065629648763766, 1.3065629648763766, 0.5411961001461970,
    -0.5411961001461970, -1.3065629648763766, -1.3065629648763766, -0.5411961001461970
};

//! Below this the tail counts as gone: -120 dB.
constexpr double silentLevel { 1.0e-6 };

double flushDenormal(double value)
{
    return std::abs(value) < 1.0e-20 ? 0.0 : value;
}

double readCubic(const std::vector<double> & buffer, size_t mask, size_t writePos, double delaySamples)
{
    const double clamped = std::clamp(delaySamples, 2.0, static_cast<double>(buffer.size() - 4));
    const auto whole = static_cast<size_t>(clamped);
    const double mu = clamped - static_cast<double>(whole);
    const double y0 = buffer[(writePos - whole + 1) & mask];
    const double y1 = buffer[(writePos - whole) & mask];
    const double y2 = buffer[(writePos - whole - 1) & mask];
    const double y3 = buffer[(writePos - whole - 2) & mask];
    const double c0 = y1;
    const double c1 = 0.5 * (y2 - y0);
    const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    return ((c3 * mu + c2) * mu + c1) * mu + c0;
}

//! The decay per band group, low (125 and 250 Hz), mid (500 Hz to 2 kHz) and high (4 and 8 kHz).
std::array<double, 3> bandGroupTimes(const Bands & times)
{
    return { std::sqrt(times[0] * times[1]), midReverbTime(times), std::sqrt(times[5] * times[6]) };
}

} // namespace

double RoomTail::Allpass::process(double input)
{
    const double buffered = buffer[writePos];
    const double stored = flushDenormal(input + buffered * coeff);
    buffer[writePos] = stored;
    if (++writePos == size) {
        writePos = 0;
    }
    return buffered - stored * coeff;
}

void RoomTail::Allpass::reset()
{
    std::ranges::fill(buffer, 0.0);
    writePos = 0;
}

double RoomTail::Line::read(double delaySamples) const
{
    return readCubic(buffer, mask, writePos, delaySamples);
}

double RoomTail::Input::read(double delaySamples) const
{
    return readCubic(buffer, mask, writePos, delaySamples);
}

void RoomTail::configure(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_smoothCoeff = 1.0 - std::exp(-1.0 / std::max(1.0, parameterSmoothSeconds * sampleRate));
    m_modDepthSamples = modDepthMs * 0.001 * sampleRate;
    const auto lineCapacity = static_cast<size_t>(std::ceil((lineLengthsMs.back() * maxLineScale + modDepthMs + 2.0) * 0.001 * sampleRate)) + 8;
    m_lineUsableCapacity = lineCapacity;
    const auto lineAlloc = std::bit_ceil(lineCapacity);
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        line.buffer.assign(lineAlloc, 0.0);
        line.mask = lineAlloc - 1;
        line.currentLength = 2.0;
        line.targetLength = 2.0;
        const double phase0 = 2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(LineCount);
        line.lfoInitRe = std::cos(phase0);
        line.lfoInitIm = std::sin(phase0);
        line.lfoRe = line.lfoInitRe;
        line.lfoIm = line.lfoInitIm;
        const double lfoInc = 2.0 * std::numbers::pi * (modBaseRateHz + modRateStepHz * static_cast<double>(i)) / sampleRate;
        line.lfoStepRe = std::cos(lfoInc);
        line.lfoStepIm = std::sin(lfoInc);
        line.lfoCounter = 0;
    }
    const auto preDelayCapacity = static_cast<size_t>(std::ceil(maxPreDelaySeconds * sampleRate)) + 8;
    m_inputUsableCapacity = preDelayCapacity;
    const auto preDelayAlloc = std::bit_ceil(preDelayCapacity);
    for (size_t i = 0; i < 2; i++) {
        auto && input = m_inputs[i];
        input.buffer.assign(preDelayAlloc, 0.0);
        input.mask = preDelayAlloc - 1;
        input.low.calculate(lowShelfHz, sampleRate);
        input.high.calculate(highShelfHz, sampleRate);
        input.lowCut.calculate(lowCutHz, sampleRate);
        const double stretch = i == 0 ? 1.0 : rightDiffuserStretch;
        for (size_t d = 0; d < DiffuserCount; d++) {
            const auto size = std::max<size_t>(1, static_cast<size_t>(std::lround(diffuserLengthsMs[d] * stretch * 0.001 * sampleRate)));
            input.diffusers[d].buffer.assign(size, 0.0);
            input.diffusers[d].size = size;
            input.diffusers[d].writePos = 0;
            input.diffusers[d].coeff = diffuserCoeffs[d];
        }
    }
    m_initialized = false;
    reset();
}

bool RoomTail::isConfigured() const
{
    return m_sampleRate > 0.0;
}

void RoomTail::reset()
{
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        std::ranges::fill(line.buffer, 0.0);
        line.writePos = 0;
        line.currentLength = line.targetLength;
        line.currentGain = line.targetGain;
        line.lfoRe = line.lfoInitRe;
        line.lfoIm = line.lfoInitIm;
        line.lfoCounter = 0;
        line.low.reset();
        line.high.reset();
    }
    for (auto && input : m_inputs) {
        std::ranges::fill(input.buffer, 0.0);
        input.writePos = 0;
        input.low.reset();
        input.high.reset();
        input.lowCut.reset();
        for (auto && diffuser : input.diffusers) {
            diffuser.reset();
        }
    }
    m_currentPreDelay = m_targetPreDelay;
    m_currentInputGain = m_targetInputGain;
    m_peak = 0.0;
    m_silentFrames = static_cast<size_t>(-1) / 2;
}

void RoomTail::set(const RoomSettings & settings, const Bands & reverbTimes, double startSeconds)
{
    if (!isConfigured()) {
        return;
    }
    const bool snap = !m_initialized || !isRinging();
    const double v = volume(settings);
    const double meanFreePathSeconds = 4.0 * v / surfaceArea(settings) / SpeedOfSound;
    double meanLengthMs = 0.0;
    for (const double ms : lineLengthsMs) {
        meanLengthMs += ms / static_cast<double>(LineCount);
    }
    const double scale = std::clamp(meanFreePathsPerLine * meanFreePathSeconds * 1000.0 / meanLengthMs, minLineScale, maxLineScale);

    const auto times = bandGroupTimes(reverbTimes);
    double meanLength = 0.0;
    const double maxLength = static_cast<double>(m_lineUsableCapacity - 6) - m_modDepthSamples;
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        line.targetLength = std::clamp(lineLengthsMs[i] * scale * 0.001 * m_sampleRate, 4.0 + m_modDepthSamples, maxLength);
        meanLength += line.targetLength / static_cast<double>(LineCount);
        // Sixty decibels down in the band's time, whatever the line's length
        const auto lineGain = [this, &line](double seconds) {
            return std::pow(10.0, -3.0 * line.targetLength / (seconds * m_sampleRate));
        };
        line.targetGain = lineGain(times[1]);
        line.lowShelf = lineGain(times[0]) / line.targetGain;
        line.highShelf = lineGain(times[2]) / line.targetGain;
        line.low.calculate(lowShelfHz, m_sampleRate);
        line.high.calculate(highShelfHz, m_sampleRate);
        if (snap) {
            line.currentLength = line.targetLength;
            line.currentGain = line.targetGain;
        }
    }

    // In from where the first line would hand the tail over to the start
    const double shortest = m_lines.front().targetLength / m_sampleRate;
    const double preDelaySeconds = std::clamp(startSeconds - shortest, 0.0, maxPreDelaySeconds);
    m_targetPreDelay = std::clamp(preDelaySeconds * m_sampleRate, 2.0, static_cast<double>(m_inputUsableCapacity - 5));

    // A unit impulse into four lines stores 4 G^2, which comes out at 4 G^2 / L a sample. That is
    // to be the room's diffuse energy at the pre-delay, per band.
    const double rate = diffuseEnergyRate(0.0, 1.0, v) / m_sampleRate;
    double inputGain = std::sqrt(rate * meanLength / 4.0);
    const auto lost = [preDelaySeconds](double seconds) {
        return std::exp(-0.5 * 13.815510557964274 * preDelaySeconds / seconds);
    };
    inputGain *= lost(times[1]);
    m_targetInputGain = inputGain;
    m_inputLowShelf = lost(times[0]) / lost(times[1]);
    m_inputHighShelf = lost(times[2]) / lost(times[1]);
    if (snap) {
        m_currentPreDelay = m_targetPreDelay;
        m_currentInputGain = m_targetInputGain;
    }
    m_initialized = true;
}

void RoomTail::process(double left, double right, double & outLeft, double & outRight)
{
    m_currentPreDelay += m_smoothCoeff * (m_targetPreDelay - m_currentPreDelay);
    m_currentInputGain += m_smoothCoeff * (m_targetInputGain - m_currentInputGain);

    // The sources in, delayed to where the tail starts, low-cut, tilted by what the room took by
    // then and diffused through four Schroeder all-passes so the FDN starts as a smooth cloud
    std::array<double, 2> in { left, right };
    for (size_t i = 0; i < 2; i++) {
        auto && input = m_inputs[i];
        input.buffer[input.writePos] = in[i];
        double delayed = input.read(m_currentPreDelay);
        input.writePos = (input.writePos + 1) & input.mask;
        input.lowCut.process(delayed);
        delayed = input.lowCut.highPass();
        input.low.process(delayed);
        delayed += (m_inputLowShelf - 1.0) * input.low.lowPass();
        input.high.process(delayed);
        delayed += (m_inputHighShelf - 1.0) * input.high.highPass();
        for (auto && diffuser : input.diffusers) {
            delayed = diffuser.process(delayed);
        }
        in[i] = delayed * m_currentInputGain;
    }

    std::array<double, LineCount> outs {};
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        line.currentLength += m_smoothCoeff * (line.targetLength - line.currentLength);
        line.currentGain += m_smoothCoeff * (line.targetGain - line.currentGain);
        const double drift = m_modDepthSamples * line.lfoIm;
        const double nextRe = line.lfoRe * line.lfoStepRe - line.lfoIm * line.lfoStepIm;
        line.lfoIm = line.lfoRe * line.lfoStepIm + line.lfoIm * line.lfoStepRe;
        line.lfoRe = nextRe;
        if ((++line.lfoCounter & 4095u) == 0u) {
            const double renorm = 0.5 * (3.0 - (line.lfoRe * line.lfoRe + line.lfoIm * line.lfoIm));
            line.lfoRe *= renorm;
            line.lfoIm *= renorm;
        }
        double out = line.read(line.currentLength + drift) * line.currentGain;
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
    constexpr double hadamardNorm = 0.5 / std::numbers::sqrt2;
    for (size_t i = 0; i < LineCount; i++) {
        auto && line = m_lines[i];
        // The left source into the even lines, the right into the odd ones
        line.buffer[line.writePos] = mixed[i] * hadamardNorm + in[i & 1] * inputSigns[i];
        line.writePos = (line.writePos + 1) & line.mask;
    }

    outLeft = 0.0;
    outRight = 0.0;
    for (size_t i = 0; i < LineCount; i++) {
        outLeft += outs[i] * leftWeights[i];
        outRight += outs[i] * rightWeights[i];
    }
    m_peak = std::max({ m_peak * 0.9995, std::abs(outLeft), std::abs(outRight) });
    m_silentFrames = std::max(std::abs(left), std::abs(right)) > silentLevel ? 0 : m_silentFrames + 1;
}

bool RoomTail::isRinging() const
{
    return m_peak > silentLevel || m_silentFrames <= static_cast<size_t>(m_targetPreDelay + m_lines.back().targetLength) + 128;
}

} // namespace noteahead
