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

#include "spectral_peak_detector.hpp"

#include "fft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! The analysis hops this many times per window.
constexpr size_t Overlap { 8 };
//! The time constant the power spectrum is averaged over. Long enough that the bins of noise stop
//! jumping out of the outline, short against how long a resonance rings.
constexpr double SmoothingSeconds { 0.03 };
//! Each bin is read together with this many neighbours either side: a sine's main lobe is about
//! four bins wide, so this costs a peak about 3 dB and noise most of its spread.
constexpr size_t SmoothingBins { 1 };
//! The outline is the level averaged over this many octaves around a bin...
constexpr double OutlineOctaves { 1.0 };
//! ...at least this many bins either side, which matters at the bottom, where an octave is a few
//! bins...
constexpr size_t MinOutlineHalfWidthBins { 8 };
//! ...without the bin's own neighbourhood, so that a peak does not lift the line it is read against.
constexpr size_t OutlineExcludedHalfWidthBins { 2 };
//! Bins further below the loudest than this, or below the floor, are not peaks but the noise floor.
constexpr double DynamicRangeDb { 60.0 };
constexpr double FloorDb { -100.0 };
//! How far Focus moves the selectivity, per octave from 1 kHz, at its ends.
constexpr double FocusDbPerOctave { 1.5 };
constexpr double FocusPivotHz { 1000.0 };
//! The selectivity never goes below this, whatever Focus does to it.
constexpr double MinSelectivityDb { 1.0 };
//! The range edges fade over this many octaves outside the limits.
constexpr double RangeFadeOctaves { 1.0 / 3.0 };
//! A reduction smaller than this is not worth a bell.
constexpr double MinReductionDb { 0.1 };

size_t fftSizeFor(double sampleRate)
{
    // About 43 ms at any rate: fine enough at the bottom for a low resonance, short enough to follow
    if (sampleRate <= 50000.0) {
        return 2048;
    }
    if (sampleRate <= 100000.0) {
        return 4096;
    }
    return 8192;
}

} // namespace

void SpectralPeakDetector::configure(double sampleRate)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_fftSize = fftSizeFor(m_sampleRate);
    m_fftMask = m_fftSize - 1;
    m_hopSize = m_fftSize / Overlap;
    const size_t bins = m_fftSize / 2 + 1;

    m_ring.assign(MaxChannels * m_fftSize, 0.0);
    m_window.resize(m_fftSize);
    for (size_t i = 0; i < m_fftSize; i++) {
        m_window[i] = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(m_fftSize));
    }
    m_input.assign(m_fftSize, 0.0);
    m_re.assign(m_fftSize, 0.0);
    m_im.assign(m_fftSize, 0.0);
    m_smoothedPower.assign(bins, 0.0);
    m_levelDb.assign(bins, FloorDb);
    m_prefixDb.assign(bins + 1, 0.0);
    m_reductionDb.assign(bins, 0.0);
    m_candidates.reserve(bins);
    m_peaks.reserve(MaxPeaks);
    m_peaks.clear();

    m_timeCoefficient = std::exp(-static_cast<double>(m_hopSize) / (SmoothingSeconds * m_sampleRate));
    // A sine of amplitude A lands in its bin at A * N / 4 through a Hann window: read as its mean
    // square, A^2 / 2
    const double n = static_cast<double>(m_fftSize);
    m_powerNormalization = 8.0 / (n * n);
    rebuildBinCache();
    reset();
}

void SpectralPeakDetector::setChannelCount(size_t channelCount)
{
    const auto count = std::clamp<size_t>(channelCount, 1, MaxChannels);
    if (count != m_channelCount) {
        m_channelCount = count;
        reset();
    }
}

void SpectralPeakDetector::setSettings(const Settings & settings)
{
    m_settings = settings;
    if (m_fftSize > 0) {
        rebuildBinCache();
    }
}

void SpectralPeakDetector::rebuildBinCache()
{
    const size_t bins = m_fftSize / 2 + 1;
    const double outlineRatio = std::pow(2.0, OutlineOctaves * 0.5);
    const double binHz = m_sampleRate / static_cast<double>(m_fftSize);

    m_binCache.resize(bins);
    m_binCache[0] = {};
    for (size_t k = 1; k < bins; k++) {
        const double frequencyHz = static_cast<double>(k) * binHz;
        const double weight = rangeWeight(frequencyHz);
        const double kd = static_cast<double>(k);
        const size_t halfWidth = std::max(MinOutlineHalfWidthBins, static_cast<size_t>(std::lround(kd * (outlineRatio - 1.0))));
        const size_t from = k > halfWidth ? k - halfWidth : 1;
        const size_t to = std::min(bins - 1, k + halfWidth);
        const size_t excludedFrom = std::max(from, k > OutlineExcludedHalfWidthBins ? k - OutlineExcludedHalfWidthBins : 1);
        const size_t excludedTo = std::min(to, k + OutlineExcludedHalfWidthBins);
        const size_t count = (to - from + 1) - (excludedTo - excludedFrom + 1);
        const double selectivityDb = std::max(MinSelectivityDb, m_settings.selectivityDb - m_settings.focus * FocusDbPerOctave * std::log2(frequencyHz / FocusPivotHz));
        m_binCache[k] = {
            .weight = weight,
            .selectivityDb = selectivityDb,
            .invCount = count > 0 ? 1.0 / static_cast<double>(count) : 0.0,
            .from = from,
            .to = to,
            .excludedFrom = excludedFrom,
            .excludedTo = excludedTo,
        };
    }
}

bool SpectralPeakDetector::push(const double * frame)
{
    if (m_fftSize == 0) {
        return false;
    }
    for (size_t channel = 0; channel < m_channelCount; channel++) {
        m_ring[channel * m_fftSize + m_writeIndex] = frame[channel];
    }
    m_writeIndex = (m_writeIndex + 1) & m_fftMask;
    if (++m_framesSinceHop >= m_hopSize) {
        m_framesSinceHop = 0;
        analyse();
        return true;
    }
    return false;
}

double SpectralPeakDetector::rangeWeight(double frequencyHz) const
{
    if (frequencyHz <= 0.0) {
        return 0.0;
    }
    const double belowOctaves = std::log2(m_settings.lowHz / frequencyHz);
    const double aboveOctaves = std::log2(frequencyHz / m_settings.highHz);
    const double outside = std::max(belowOctaves, aboveOctaves);
    return outside <= 0.0 ? 1.0 : std::max(0.0, 1.0 - outside / RangeFadeOctaves);
}

void SpectralPeakDetector::analyse()
{
    const size_t bins = m_fftSize / 2 + 1;
    const double normalization = m_powerNormalization / static_cast<double>(m_channelCount);
    const double smoothing = m_timeCoefficient;

    // The power spectrum, the channels summed, averaged over time
    for (size_t k = 0; k < bins; k++) {
        m_smoothedPower[k] *= smoothing;
    }
    for (size_t channel = 0; channel < m_channelCount; channel++) {
        const double * ring = m_ring.data() + channel * m_fftSize;
        const size_t firstLen = m_fftSize - m_writeIndex;
        for (size_t i = 0; i < firstLen; i++) {
            m_input[i] = ring[m_writeIndex + i] * m_window[i];
        }
        for (size_t i = 0; i < m_writeIndex; i++) {
            m_input[firstLen + i] = ring[i] * m_window[firstLen + i];
        }
        Fft::forwardReal(m_input.data(), m_re.data(), m_im.data(), static_cast<int>(m_fftSize));
        for (size_t k = 0; k < bins; k++) {
            const double power = (m_re[k] * m_re[k] + m_im[k] * m_im[k]) * normalization;
            m_smoothedPower[k] += power * (1.0 - smoothing);
        }
    }

    // In dB, each bin read with its neighbours
    double loudestDb = FloorDb;
    for (size_t k = 0; k < bins; k++) {
        const size_t from = k > SmoothingBins ? k - SmoothingBins : 0;
        const size_t to = std::min(bins - 1, k + SmoothingBins);
        double sum = 0.0;
        for (size_t j = from; j <= to; j++) {
            sum += m_smoothedPower[j];
        }
        m_levelDb[k] = std::max(FloorDb, 10.0 * std::log10(sum / static_cast<double>(to - from + 1) + 1.0e-30));
        loudestDb = std::max(loudestDb, m_levelDb[k]);
    }
    m_prefixDb[0] = 0.0;
    for (size_t k = 0; k < bins; k++) {
        m_prefixDb[k + 1] = m_prefixDb[k] + m_levelDb[k];
    }
    const auto sumDb = [this](size_t from, size_t to) { // Inclusive
        return m_prefixDb[to + 1] - m_prefixDb[from];
    };

    // Each bin against the outline
    const double floorDb = std::max(FloorDb, loudestDb - DynamicRangeDb);
    const double binHz = m_sampleRate / static_cast<double>(m_fftSize);
    m_candidates.clear();
    m_reductionDb[0] = 0.0;
    for (size_t k = 1; k < bins; k++) {
        m_reductionDb[k] = 0.0;
        const auto & bc = m_binCache[k];
        if (bc.weight <= 0.0 || bc.invCount <= 0.0 || m_levelDb[k] <= floorDb) {
            continue;
        }
        const double outlineDb = (sumDb(bc.from, bc.to) - sumDb(bc.excludedFrom, bc.excludedTo)) * bc.invCount;
        const double excessDb = m_levelDb[k] - outlineDb - bc.selectivityDb;
        m_reductionDb[k] = std::clamp(excessDb, 0.0, m_settings.depthDb) * bc.weight;
    }

    // The strongest local maxima, far enough apart to be peaks of their own
    for (size_t k = 1; k + 1 < bins; k++) {
        const double r = m_reductionDb[k];
        if (r >= MinReductionDb && r >= m_reductionDb[k - 1] && r > m_reductionDb[k + 1]) {
            m_candidates.push_back({ k, r });
        }
    }
    std::sort(m_candidates.begin(), m_candidates.end(), [](const Candidate & a, const Candidate & b) {
        return a.reductionDb > b.reductionDb;
    });
    m_peaks.clear();
    for (auto && candidate : m_candidates) {
        if (m_peaks.size() >= MaxPeaks) {
            break;
        }
        // Between the bins, from the parabola through the level and its neighbours
        const size_t k = candidate.bin;
        const double a = m_levelDb[k - 1];
        const double b = m_levelDb[k];
        const double c = m_levelDb[k + 1];
        const double denominator = a - 2.0 * b + c;
        const double offset = std::abs(denominator) > 1.0e-12 ? std::clamp(0.5 * (a - c) / denominator, -0.5, 0.5) : 0.0;
        const double frequencyHz = (static_cast<double>(k) + offset) * binHz;
        const bool tooClose = std::any_of(m_peaks.begin(), m_peaks.end(), [&](const Peak & peak) {
            return std::abs(std::log2(peak.frequencyHz / frequencyHz)) < m_settings.minSeparationOctaves;
        });
        if (!tooClose) {
            m_peaks.push_back({ frequencyHz, candidate.reductionDb });
        }
    }
}

std::span<const SpectralPeakDetector::Peak> SpectralPeakDetector::peaks() const
{
    return m_peaks;
}

std::span<const double> SpectralPeakDetector::reductionsDb() const
{
    return m_reductionDb;
}

double SpectralPeakDetector::binFrequencyHz(size_t bin) const
{
    return m_fftSize ? static_cast<double>(bin) * m_sampleRate / static_cast<double>(m_fftSize) : 0.0;
}

size_t SpectralPeakDetector::fftSize() const
{
    return m_fftSize;
}

size_t SpectralPeakDetector::hopSize() const
{
    return m_hopSize;
}

void SpectralPeakDetector::reset()
{
    std::fill(m_ring.begin(), m_ring.end(), 0.0);
    std::fill(m_smoothedPower.begin(), m_smoothedPower.end(), 0.0);
    std::fill(m_levelDb.begin(), m_levelDb.end(), FloorDb);
    std::fill(m_reductionDb.begin(), m_reductionDb.end(), 0.0);
    m_peaks.clear();
    m_writeIndex = 0;
    m_framesSinceHop = 0;
}

} // namespace noteahead
