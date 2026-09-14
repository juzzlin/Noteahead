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

#include "spectrum_analyzer.hpp"

#include "../dsp/fft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Long enough to resolve the bottom third-octave band, which is what sets it: at 44.1 kHz this is
//! about 2.7 Hz per bin, and the 25 Hz band is only 5.8 Hz wide.
constexpr size_t FrameSize = 16384;

//! Half-overlapped, so that a Hann window does not lose the material that lands on its skirts.
constexpr size_t HopSize = FrameSize / 2;

//! Frames quieter than this contribute nothing.
//!
//! A render carries lead-in silence, a fade and a tail by design, and averaging those in drags the
//! whole curve down without saying anything about the mix. The gate is deliberately low: it is here
//! to drop silence, not to pick out the loud parts.
constexpr double SilenceGateDb = -40.0;

//! The span every level is reported relative to.
//!
//! Wide enough that no single instrument decides where the zero sits, and it stops below the region
//! where a mix's own tilt takes over, so what is left after normalising is balance and not slope.
constexpr double NormalizeLowHz = 99.0;
constexpr double NormalizeHighHz = 8000.0;

//! The four regions the summary reports, as [low, high] in hertz.
constexpr double LowMidRange[] { 99.0, 250.0 };
constexpr double MidRange[] { 250.0, 630.0 };
constexpr double UpperMidRange[] { 794.0, 1600.0 };
constexpr double HighRange[] { 2520.0, 8000.0 };

//! Third-octave band edges: a sixth of an octave either side of the centre.
constexpr double EdgeRatio = 1.122462048309373; // 2^(1/6)

double meanOfRange(const std::vector<SpectrumAnalyzer::Band> & bands, double lowHz, double highHz)
{
    double sum = 0.0;
    size_t count = 0;
    for (const auto & band : bands) {
        if (band.centerHz >= lowHz && band.centerHz <= highHz) {
            sum += static_cast<double>(band.levelDb);
            count++;
        }
    }
    return count ? sum / static_cast<double>(count) : 0.0;
}

} // namespace

SpectrumAnalyzer::SpectrumAnalyzer(double sampleRate, int channels)
  : m_sampleRate { sampleRate > 0.0 ? sampleRate : 48000.0 }
  , m_channels { std::max(1, channels) }
  , m_window(FrameSize)
  , m_frame(FrameSize, 0.0)
  , m_powerSum(FrameSize / 2 + 1, 0.0)
{
    for (size_t i = 0; i < FrameSize; i++) {
        m_window[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(FrameSize - 1)));
    }
}

void SpectrumAnalyzer::process(const float * data, size_t numSamples)
{
    if (!data) {
        return;
    }

    const auto stride = static_cast<size_t>(m_channels);
    for (size_t i = 0; i + stride <= numSamples; i += stride) {
        // Summed to mono, which is both what a balance reading wants and what makes the result say
        // something about the mix as it survives a mono playback system.
        double mono = 0.0;
        for (size_t channel = 0; channel < stride; channel++) {
            mono += static_cast<double>(data[i + channel]);
        }
        pushSample(mono / static_cast<double>(stride));
    }
}

void SpectrumAnalyzer::pushSample(double mono)
{
    m_frame[m_frameFill++] = mono;
    if (m_frameFill < FrameSize) {
        return;
    }

    analyzeFrame();

    // Slide the window on by one hop, keeping the overlap.
    std::copy(m_frame.begin() + static_cast<long>(HopSize), m_frame.end(), m_frame.begin());
    m_frameFill = FrameSize - HopSize;
}

void SpectrumAnalyzer::analyzeFrame()
{
    double sumSquares = 0.0;
    for (size_t i = 0; i < FrameSize; i++) {
        sumSquares += m_frame[i] * m_frame[i];
    }
    const double rms = std::sqrt(sumSquares / static_cast<double>(FrameSize));
    if (20.0 * std::log10(std::max(rms, 1.0e-12)) < SilenceGateDb) {
        return;
    }

    std::vector<double> re(FrameSize);
    std::vector<double> im(FrameSize, 0.0);
    for (size_t i = 0; i < FrameSize; i++) {
        re[i] = m_frame[i] * m_window[i];
    }

    Fft::forward(re.data(), im.data(), static_cast<int>(FrameSize));

    for (size_t bin = 0; bin < m_powerSum.size(); bin++) {
        m_powerSum[bin] += re[bin] * re[bin] + im[bin] * im[bin];
    }
    m_frameCount++;
}

std::vector<double> SpectrumAnalyzer::bandCenters()
{
    std::vector<double> centers;
    // 25 Hz to about 16 kHz, the span a mix is judged over.
    for (int third = -16; third <= 12; third++) {
        centers.push_back(1000.0 * std::pow(2.0, static_cast<double>(third) / 3.0));
    }
    return centers;
}

SpectrumAnalyzer::Result SpectrumAnalyzer::calculate() const
{
    Result result;
    if (!m_frameCount) {
        return result;
    }

    const double binHz = m_sampleRate / static_cast<double>(FrameSize);
    const double nyquist = m_sampleRate * 0.5;

    for (auto && center : bandCenters()) {
        const double low = center / EdgeRatio;
        const double high = center * EdgeRatio;
        if (low >= nyquist) {
            break;
        }
        double power = 0.0;
        const auto firstBin = static_cast<size_t>(std::ceil(low / binHz));
        for (size_t bin = firstBin; bin < m_powerSum.size(); bin++) {
            if (static_cast<double>(bin) * binHz >= high) {
                break;
            }
            power += m_powerSum[bin];
        }
        // Averaged over the frames that passed the gate, so a long file and a short one read alike.
        power /= static_cast<double>(m_frameCount);
        result.bands.push_back(Band { center, static_cast<float>(10.0 * std::log10(std::max(power, 1.0e-30))) });
    }

    if (result.bands.empty()) {
        return result;
    }

    // Everything is reported against the mix's own midrange average: that is what removes both its
    // mastering level and its gross tilt, and leaves two mixes comparable band for band.
    const double reference = meanOfRange(result.bands, NormalizeLowHz, NormalizeHighHz);
    for (auto & band : result.bands) {
        band.levelDb = static_cast<float>(static_cast<double>(band.levelDb) - reference);
    }

    result.lowMidDb = static_cast<float>(meanOfRange(result.bands, LowMidRange[0], LowMidRange[1]));
    result.midDb = static_cast<float>(meanOfRange(result.bands, MidRange[0], MidRange[1]));
    result.upperMidDb = static_cast<float>(meanOfRange(result.bands, UpperMidRange[0], UpperMidRange[1]));
    result.highDb = static_cast<float>(meanOfRange(result.bands, HighRange[0], HighRange[1]));
    result.upperMidToHighDb = result.upperMidDb - result.highDb;
    result.isValid = true;

    return result;
}

} // namespace noteahead
