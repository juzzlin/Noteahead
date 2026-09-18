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

#include "rta.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"
#include "../dsp/fft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Bars each band-count mode asks for, and the longest window it needs to place the lowest of them.
//!
//! Appended to rather than reordered: the mode is what projects store, so 96 lands after 128 even
//! though it belongs between 64 and 128 on the panel. The dialog puts it back in order.
constexpr int BandCounts[] = { 32, 64, 128, 96 };
constexpr int LongestFftSizes[] = { 8192, 16384, 32768, 32768 };
constexpr int MaxBands = 128;

//! Overlap of every tier, by FFT Rate mode: how many analyses one window's worth of audio gets.
constexpr int OverlapFactors[] = { 32, 16, 8 };

//! Analyses per second every tier runs at least, by FFT Rate mode.
//!
//! Overlap alone would leave the longest window updating twenty times a second, and a bar that
//! moves twenty times a second reads as a stutter however smoothly it is drawn. This is the floor
//! that keeps the bottom of the display moving at the rate the eye follows the top at; the shorter
//! windows are already far past it on overlap alone.
constexpr double MinUpdatesPerSecond[] = { 60.0, 40.0, 20.0 };

} // namespace

Rta::Rta()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyBandCount().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDbRange().toStdString(), 2.0f, 0, 3, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyShowPinkNoise().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPinkNoiseLevel().toStdString(), -18.0f, -80, 0, -18, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySpeed().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFftRate().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });

    // Allocated once, at the largest any mode can ask for, and used from the front afterwards: a
    // rebuild happens on the audio thread, which must not allocate.
    int capacity = MaxTierFftSize;
    for (auto & tier : m_tiers) {
        tier.window.assign(capacity, 0.0);
        tier.inBuf.assign(capacity, 0.0);
        tier.timeBuf.assign(capacity, 0.0);
        tier.fftRe.assign(capacity, 0.0);
        tier.fftIm.assign(capacity, 0.0);
        tier.bands.reserve(MaxBands);
        capacity /= TierSizeRatio;
    }

    m_smoothedPow.reserve(MaxBands);
    m_bandBins.reserve(MaxBands);
    m_bandWeights.reserve(MaxBandWeights);
    m_bandWeightOffsets.reserve(MaxBands);
    m_bandLogX.reserve(MaxBands);
    m_bandUpdates.reserve(MaxBands);
    m_bandDb.reserve(MaxBands);
    m_bandLogXPublic.reserve(MaxBands);

    Rta::syncParameters();
}

std::string Rta::typeIdString()
{
    return "b9f2e4d7-3a8c-4e6b-9f1d-2c5e7a0b3d4f";
}

std::string Rta::type() const
{
    return Constants::RackEffectType::rta().toStdString();
}

std::string Rta::typeId() const
{
    return typeIdString();
}

void Rta::setAnalysisEnabled(bool enabled)
{
    m_analysisEnabled.store(enabled, std::memory_order_relaxed);
}

std::vector<float> Rta::bandMagnitudesDb() const
{
    const std::lock_guard<std::mutex> lock { m_bandMutex };
    return m_bandDb;
}

std::vector<std::pair<float, float>> Rta::bandLogPositions() const
{
    const std::lock_guard<std::mutex> lock { m_bandMutex };
    return m_bandLogXPublic;
}

uint32_t Rta::layoutGeneration() const
{
    return m_layoutGeneration.load(std::memory_order_acquire);
}

void Rta::buildTierWindow(Tier & tier)
{
    for (int i = 0; i < tier.fftN; i++) {
        tier.window[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (tier.fftN - 1)));
    }
}

void Rta::buildBands()
{
    const int mode = std::clamp(m_bandCountMode, 0, 3);

    // The longest window scales with band count, and each tier below it is half of the one above:
    // six resolutions, spanning a factor of 32 in both window length and response time.
    int requestedFftN = LongestFftSizes[mode];
    for (auto & tier : m_tiers) {
        const int newFftN = std::clamp(requestedFftN, MinTierFftSize, static_cast<int>(tier.inBuf.size()));
        if (newFftN != tier.fftN) {
            tier.fftN = newFftN;
            tier.specBins = newFftN / 2 + 1;
            tier.hopFill = 0;
            tier.writePos = 0;
            std::fill(tier.inBuf.begin(), tier.inBuf.end(), 0.0);
            buildTierWindow(tier);
        }
        tier.bands.clear();
        requestedFftN /= TierSizeRatio;
    }

    const int requestedB = BandCounts[mode];
    const double sr = m_sampleRateCached;
    static constexpr double logRange = std::log10(FreqHi / FreqLo);

    m_bandBins.clear();
    m_bandLogX.clear();
    m_bandWeights.clear();
    m_bandWeightOffsets.clear();

    // Which single bin the bar before this one settled for, per resolution, and at which
    // resolution: that is the only case where two bars can turn out to be the same measurement.
    std::array<int, TierCount> lastSingleBin {};
    lastSingleBin.fill(-1);
    int previousTier = -1;

    for (int b = 0; b < requestedB; b++) {
        const double fLo = FreqLo * std::pow(FreqHi / FreqLo, static_cast<double>(b) / requestedB);
        const double fHi = FreqLo * std::pow(FreqHi / FreqLo, static_cast<double>(b + 1) / requestedB);

        const float xLo = static_cast<float>(std::log10(fLo / FreqLo) / logRange);
        const float xHi = static_cast<float>(std::log10(fHi / FreqLo) / logRange);

        // The shortest window that still puts MinBinsPerBand bins inside this band. Tiers are
        // ordered longest first, so walking them backwards finds the fastest one that will do.
        int tierIndex = 0;
        for (int k = TierCount - 1; k > 0; k--) {
            if (sr / m_tiers[k].fftN <= (fHi - fLo) / MinBinsPerBand) {
                tierIndex = k;
                break;
            }
        }

        auto & tier = m_tiers[tierIndex];
        const double binHz = sr / tier.fftN;

        // A bin covers half a bin either side of its centre, so the band reaches every bin whose
        // coverage it overlaps at all rather than only those centred inside it.
        const int kLo = std::max(1, static_cast<int>(std::floor(fLo / binHz + 0.5)));
        if (kLo > tier.specBins - 1) {
            break;
        }
        const int kHi = std::max(kLo, std::min(tier.specBins - 1, static_cast<int>(std::ceil(fHi / binHz - 0.5))));

        // Weights before anything is recorded: a band the weights cannot be stored for is better
        // left out than drawn from someone else's.
        if (static_cast<int>(m_bandWeights.size()) + (kHi - kLo + 1) > MaxBandWeights) {
            break;
        }

        const int weightOffset = static_cast<int>(m_bandWeights.size());
        double weightSum = 0.0;
        for (int k = kLo; k <= kHi; k++) {
            const double overlap = std::min(fHi, (k + 0.5) * binHz) - std::max(fLo, (k - 0.5) * binHz);
            const double weight = std::clamp(overlap / binHz, 0.0, 1.0);
            m_bandWeights.push_back(weight);
            weightSum += weight;
        }

        // Narrower than a bin, which is where the bottom of a log display always ends up: there is
        // nothing to apportion, and a fraction of a bin would read the band low. It takes the bin it
        // sits in, whole, the way it did before there were weights to give.
        int singleBin = -1;
        if (weightSum < 1.0) {
            singleBin = std::clamp(static_cast<int>(std::lround(std::sqrt(fLo * fHi) / binHz)), kLo, kHi);
            for (int k = kLo; k <= kHi; k++) {
                m_bandWeights[weightOffset + k - kLo] = (k == singleBin) ? 1.0 : 0.0;
            }
        }

        // Two bars reading the same single bin would only ever draw the same height, so the earlier
        // one widens to cover both instead. Bands wide enough to apportion bins are never copies of
        // each other, however much of an edge bin they share.
        if (singleBin >= 0 && singleBin == lastSingleBin[tierIndex] && tierIndex == previousTier && !m_bandBins.empty()) {
            m_bandWeights.resize(weightOffset);
            m_bandLogX.back().second = xHi;
            continue;
        }

        lastSingleBin[tierIndex] = singleBin;
        previousTier = tierIndex;
        tier.bands.push_back(static_cast<int>(m_bandBins.size()));
        m_bandBins.push_back({ kLo, kHi });
        m_bandWeightOffsets.push_back(weightOffset);
        m_bandLogX.push_back({ xLo, xHi });
    }

    const int actualB = static_cast<int>(m_bandBins.size());
    m_smoothedPow.assign(actualB, 0.0);
    {
        const std::lock_guard<std::mutex> lock { m_bandMutex };
        m_bandDb.assign(actualB, -100.0f);
        m_bandLogXPublic = m_bandLogX;
    }
    m_layoutGeneration.fetch_add(1, std::memory_order_release);
}

void Rta::smoothingCoefficients(const Tier & tier, double & attackCoeff, double & releaseCoeff) const
{
    double attackMs = 10.0, releaseMs = 300.0;
    if (m_speedMode == 0) {
        attackMs = 5.0;
        releaseMs = 80.0;
    } else if (m_speedMode == 2) {
        attackMs = 30.0;
        releaseMs = 800.0;
    }

    // A shorter window forgets sooner, so its bars would fall faster than their neighbours' for no
    // reason the eye can attribute to the signal. Stretching the release by the difference in
    // window length keeps one decay across the whole display.
    releaseMs += static_cast<double>(m_tiers[0].fftN - tier.fftN) / (2.0 * m_sampleRateCached) * 1000.0;

    const double sr = m_sampleRateCached;
    attackCoeff = std::exp(-static_cast<double>(tier.hopSize) / (sr * attackMs / 1000.0));
    releaseCoeff = std::exp(-static_cast<double>(tier.hopSize) / (sr * releaseMs / 1000.0));
}

void Rta::runTierAnalysis(Tier & tier)
{
    if (tier.bands.empty()) {
        return;
    }

    // The input is circular, so the window is applied while gathering rather than by shifting the
    // whole buffer down every hop. The oldest sample is the one about to be overwritten.
    const int tail = tier.fftN - tier.writePos;
    for (int i = 0; i < tail; i++) {
        tier.timeBuf[i] = tier.inBuf[tier.writePos + i] * tier.window[i];
    }
    for (int i = 0; i < tier.writePos; i++) {
        tier.timeBuf[tail + i] = tier.inBuf[i] * tier.window[tail + i];
    }

    // Real input, so half a transform is all it takes: the bins above Nyquist mirror the ones below
    // and the band sums never look at them.
    Fft::forwardReal(tier.timeBuf.data(), tier.fftRe.data(), tier.fftIm.data(), tier.fftN);

    double attackCoeff = 0.0, releaseCoeff = 0.0;
    smoothingCoefficients(tier, attackCoeff, releaseCoeff);

    const double scale = 1.0 / (tier.fftN * 0.5);

    auto & updates = m_bandUpdates;
    updates.clear();
    for (const int b : tier.bands) {
        const auto [kLo, kHi] = m_bandBins[b];
        const int weightOffset = m_bandWeightOffsets[b];
        double sumPow = 0.0;
        for (int k = kLo; k <= kHi; k++) {
            const double power = tier.fftRe[k] * tier.fftRe[k] + tier.fftIm[k] * tier.fftIm[k];
            sumPow += m_bandWeights[weightOffset + k - kLo] * power * scale * scale;
        }
        const double coeff = (sumPow > m_smoothedPow[b]) ? attackCoeff : releaseCoeff;
        m_smoothedPow[b] = coeff * m_smoothedPow[b] + (1.0 - coeff) * sumPow;
        updates.push_back({ b, static_cast<float>(10.0 * std::log10(m_smoothedPow[b] + 1e-20)) });
    }

    {
        const std::lock_guard<std::mutex> lock { m_bandMutex };
        for (const auto & [idx, db] : updates) {
            m_bandDb[idx] = db;
        }
    }
}

void Rta::processSample(double &, double &)
{
}

void Rta::processBlock(AudioContext & context)
{
    if (!m_analysisEnabled.load(std::memory_order_relaxed)) {
        return;
    }

    if (m_shouldSync) {
        syncParameters();
    }

    if (context.sampleRate != m_lastSampleRate) {
        m_lastSampleRate = context.sampleRate;
        m_sampleRateCached = static_cast<double>(context.sampleRate);
        buildBands();
        syncParameters();
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const double mono = (context.buffer[i * 2] + context.buffer[i * 2 + 1]) * 0.5;

        for (auto & tier : m_tiers) {
            tier.inBuf[tier.writePos] = mono;
            tier.writePos++;
            if (tier.writePos >= tier.fftN) {
                tier.writePos = 0;
            }
            tier.hopFill++;
            if (tier.hopFill >= tier.hopSize) {
                tier.hopFill = 0;
                runTierAnalysis(tier);
            }
        }
    }
}

void Rta::reset()
{
    for (auto & tier : m_tiers) {
        std::fill(tier.inBuf.begin(), tier.inBuf.end(), 0.0);
        tier.hopFill = 0;
        tier.writePos = 0;
    }
    std::fill(m_smoothedPow.begin(), m_smoothedPow.end(), 0.0);
    {
        const std::lock_guard<std::mutex> lock { m_bandMutex };
        std::fill(m_bandDb.begin(), m_bandDb.end(), -100.0f);
    }
}

void Rta::sync()
{
    m_shouldSync = true;
}

void Rta::syncParameters()
{
    m_shouldSync = false;
    bool needRebuild = false;

    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandCount().toStdString()); p) {
        const int newMode = std::clamp(static_cast<int>(std::round(p->get().value())), 0, 3);
        if (newMode != m_bandCountMode) {
            m_bandCountMode = newMode;
            needRebuild = true;
        }
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDbRange().toStdString()); p) {
        m_dbRangeMode = std::clamp(static_cast<int>(std::round(p->get().value())), 0, 3);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyShowPinkNoise().toStdString()); p) {
        m_showPinkNoise = p->get().value() >= 0.5f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPinkNoiseLevel().toStdString()); p) {
        m_pinkNoiseLevel = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySpeed().toStdString()); p) {
        m_speedMode = std::clamp(static_cast<int>(std::round(p->get().value())), 0, 2);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFftRate().toStdString()); p) {
        m_fftRateMode = std::clamp(static_cast<int>(std::round(p->get().value())), 0, 2);
    }

    if (needRebuild || m_bandBins.empty()) {
        buildBands(); // Sets the tier window lengths the hop sizes below are derived from.
    }

    const int maxHopSize = std::max(MinHopSize, static_cast<int>(m_sampleRateCached / MinUpdatesPerSecond[m_fftRateMode]));
    for (auto & tier : m_tiers) {
        const int newHopSize = std::clamp(tier.fftN / OverlapFactors[m_fftRateMode], MinHopSize, maxHopSize);
        if (newHopSize != tier.hopSize) {
            tier.hopSize = newHopSize;
            tier.hopFill = 0;
        }
    }
}

} // namespace noteahead
