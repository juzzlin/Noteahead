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

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

Rta::Rta()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyBandCount().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDbRange().toStdString(), 2.0f, 0, 3, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyShowPinkNoise().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPinkNoiseLevel().toStdString(), -18.0f, -80, 0, -18, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySpeed().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFftRate().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });

    m_slowInBuf.fill(0.0);
    m_slowFftRe.fill(0.0);
    m_slowFftIm.fill(0.0);
    m_fastInBuf.fill(0.0);
    m_fastFftRe.fill(0.0);
    m_fastFftIm.fill(0.0);

    buildWindows();
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

void Rta::buildWindows()
{
    for (int i = 0; i < m_slowFftN; i++) {
        m_slowWindow[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (m_slowFftN - 1)));
    }
    for (int i = 0; i < m_fastFftN; i++) {
        m_fastWindow[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (m_fastFftN - 1)));
    }
}

void Rta::buildBands()
{
    // FFT sizes scale with band count so CPU scales proportionally.
    static constexpr int slowFftSizes[] = { 8192, 16384, 32768 };
    const int newSlowFftN = slowFftSizes[std::clamp(m_bandCountMode, 0, 2)];
    const int newFastFftN = newSlowFftN / 4;

    if (newSlowFftN != m_slowFftN || newFastFftN != m_fastFftN) {
        m_slowFftN = newSlowFftN;
        m_fastFftN = newFastFftN;
        m_slowSpecBins = m_slowFftN / 2 + 1;
        m_fastSpecBins = m_fastFftN / 2 + 1;
        m_slowInBuf.fill(0.0);
        m_fastInBuf.fill(0.0);
        m_slowHopFill = 0;
        m_fastHopFill = 0;
        buildWindows();
    }

    const int requestedB = [&] {
        static constexpr int counts[] = { 32, 64, 128 };
        return counts[std::clamp(m_bandCountMode, 0, 2)];
    }();
    const double sr = m_sampleRateCached;
    static constexpr double logRange = std::log10(FreqHi / FreqLo);

    m_bandBins.clear();
    m_bandFast.clear();
    m_bandLogX.clear();
    m_bandBins.reserve(requestedB);
    m_bandFast.reserve(requestedB);
    m_bandLogX.reserve(requestedB);

    int lastSlowKHi = 0;
    int lastFastKHi = 0;

    for (int b = 0; b < requestedB; b++) {
        const double fLo = FreqLo * std::pow(FreqHi / FreqLo, static_cast<double>(b) / requestedB);
        const double fHi = FreqLo * std::pow(FreqHi / FreqLo, static_cast<double>(b + 1) / requestedB);
        const double fCenter = std::sqrt(fLo * fHi);

        const float xLo = static_cast<float>(std::log10(fLo / FreqLo) / logRange);
        const float xHi = static_cast<float>(std::log10(fHi / FreqLo) / logRange);

        const bool useFast = (fCenter >= CrossoverFreq);

        if (useFast) {
            int kLo = std::max(1, static_cast<int>(std::ceil(fLo * m_fastFftN / sr)));
            int kHi = std::min(m_fastSpecBins - 1, static_cast<int>(std::floor(fHi * m_fastFftN / sr)));
            kHi = std::max(kLo, kHi);
            if (kLo > m_fastSpecBins - 1) {
                break;
            }
            if (kLo <= lastFastKHi && !m_bandBins.empty() && m_bandFast.back()) {
                m_bandLogX.back().second = xHi;
                continue;
            }
            lastFastKHi = kHi;
            m_bandBins.push_back({ kLo, kHi });
            m_bandFast.push_back(true);
            m_bandLogX.push_back({ xLo, xHi });
        } else {
            int kLo = std::max(1, static_cast<int>(std::ceil(fLo * m_slowFftN / sr)));
            int kHi = std::min(m_slowSpecBins - 1, static_cast<int>(std::floor(fHi * m_slowFftN / sr)));
            kHi = std::max(kLo, kHi);
            if (kLo > m_slowSpecBins - 1) {
                break;
            }
            if (kLo <= lastSlowKHi && !m_bandBins.empty() && !m_bandFast.back()) {
                m_bandLogX.back().second = xHi;
                continue;
            }
            lastSlowKHi = kHi;
            m_bandBins.push_back({ kLo, kHi });
            m_bandFast.push_back(false);
            m_bandLogX.push_back({ xLo, xHi });
        }
    }

    const int actualB = static_cast<int>(m_bandBins.size());
    m_smoothedPow.assign(actualB, 0.0);
    {
        const std::lock_guard<std::mutex> lock { m_bandMutex };
        m_bandDb.assign(actualB, -100.0f);
        m_bandLogXPublic = m_bandLogX;
    }
}

// Iterative Cooley-Tukey DIT radix-2 FFT (in-place, N must be a power of 2).
void Rta::fft(double * re, double * im, int N)
{
    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }

    for (int len = 2; len <= N; len <<= 1) {
        const double ang = -2.0 * std::numbers::pi / len;
        const double wStepRe = std::cos(ang);
        const double wStepIm = std::sin(ang);
        for (int i = 0; i < N; i += len) {
            double wRe = 1.0;
            double wIm = 0.0;
            for (int j = 0; j < len / 2; j++) {
                const double uRe = re[i + j];
                const double uIm = im[i + j];
                const double vRe = re[i + j + len / 2] * wRe - im[i + j + len / 2] * wIm;
                const double vIm = re[i + j + len / 2] * wIm + im[i + j + len / 2] * wRe;
                re[i + j] = uRe + vRe;
                im[i + j] = uIm + vIm;
                re[i + j + len / 2] = uRe - vRe;
                im[i + j + len / 2] = uIm - vIm;
                const double newWRe = wRe * wStepRe - wIm * wStepIm;
                wIm = wRe * wStepIm + wIm * wStepRe;
                wRe = newWRe;
            }
        }
    }
}

void Rta::runSlowAnalysis()
{
    const int B = static_cast<int>(m_bandBins.size());
    if (B == 0) {
        return;
    }

    const double scale = 1.0 / (m_slowFftN * 0.5);
    for (int i = 0; i < m_slowFftN; i++) {
        m_slowFftRe[i] = m_slowInBuf[i] * m_slowWindow[i];
        m_slowFftIm[i] = 0.0;
    }
    fft(m_slowFftRe.data(), m_slowFftIm.data(), m_slowFftN);

    const double sr = m_sampleRateCached;
    double attackMs = 10.0, releaseMs = 300.0;
    if (m_speedMode == 0) {
        attackMs = 5.0;
        releaseMs = 80.0;
    } else if (m_speedMode == 2) {
        attackMs = 30.0;
        releaseMs = 800.0;
    }
    const double attackCoeff = std::exp(-static_cast<double>(SlowHopSize) / (sr * attackMs / 1000.0));
    const double releaseCoeff = std::exp(-static_cast<double>(SlowHopSize) / (sr * releaseMs / 1000.0));

    std::vector<std::pair<int, float>> updates;
    for (int b = 0; b < B; b++) {
        if (m_bandFast[b]) {
            continue;
        }
        const auto [kLo, kHi] = m_bandBins[b];
        double sumPow = 0.0;
        for (int k = kLo; k <= kHi; k++) {
            const double amp = scale * std::sqrt(m_slowFftRe[k] * m_slowFftRe[k] + m_slowFftIm[k] * m_slowFftIm[k]);
            sumPow += amp * amp;
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

void Rta::runFastAnalysis()
{
    const int B = static_cast<int>(m_bandBins.size());
    if (B == 0) {
        return;
    }

    const double scale = 1.0 / (m_fastFftN * 0.5);
    for (int i = 0; i < m_fastFftN; i++) {
        m_fastFftRe[i] = m_fastInBuf[i] * m_fastWindow[i];
        m_fastFftIm[i] = 0.0;
    }
    fft(m_fastFftRe.data(), m_fastFftIm.data(), m_fastFftN);

    const double sr = m_sampleRateCached;
    double attackMs = 10.0, releaseMs = 300.0;
    if (m_speedMode == 0) {
        attackMs = 5.0;
        releaseMs = 80.0;
    } else if (m_speedMode == 2) {
        attackMs = 30.0;
        releaseMs = 800.0;
    }
    // Extend fast-band release to compensate for shorter window losing history sooner.
    const double windowDeltaMs = static_cast<double>(m_slowFftN - m_fastFftN) / (2.0 * sr) * 1000.0;
    const double attackCoeff = std::exp(-static_cast<double>(m_fastHopSize) / (sr * attackMs / 1000.0));
    const double releaseCoeff = std::exp(-static_cast<double>(m_fastHopSize) / (sr * (releaseMs + windowDeltaMs) / 1000.0));

    std::vector<std::pair<int, float>> updates;
    for (int b = 0; b < B; b++) {
        if (!m_bandFast[b]) {
            continue;
        }
        const auto [kLo, kHi] = m_bandBins[b];
        double sumPow = 0.0;
        for (int k = kLo; k <= kHi; k++) {
            const double amp = scale * std::sqrt(m_fastFftRe[k] * m_fastFftRe[k] + m_fastFftIm[k] * m_fastFftIm[k]);
            sumPow += amp * amp;
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

void Rta::process(double &, double &)
{
}

void Rta::process(AudioContext & context)
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
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const double mono = (context.buffer[i * 2] + context.buffer[i * 2 + 1]) * 0.5;

        // Slow FFT — LF bands
        m_slowInBuf[m_slowFftN - SlowHopSize + m_slowHopFill] = mono;
        m_slowHopFill++;
        if (m_slowHopFill >= SlowHopSize) {
            m_slowHopFill = 0;
            runSlowAnalysis();
            std::copy(m_slowInBuf.data() + SlowHopSize, m_slowInBuf.data() + m_slowFftN, m_slowInBuf.data());
        }

        // Fast FFT — HF bands
        m_fastInBuf[m_fastFftN - m_fastHopSize + m_fastHopFill] = mono;
        m_fastHopFill++;
        if (m_fastHopFill >= m_fastHopSize) {
            m_fastHopFill = 0;
            runFastAnalysis();
            std::copy(m_fastInBuf.data() + m_fastHopSize, m_fastInBuf.data() + m_fastFftN, m_fastInBuf.data());
        }
    }
}

void Rta::reset()
{
    std::fill(m_slowInBuf.data(), m_slowInBuf.data() + m_slowFftN, 0.0);
    std::fill(m_fastInBuf.data(), m_fastInBuf.data() + m_fastFftN, 0.0);
    m_slowHopFill = 0;
    m_fastHopFill = 0;
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
        const int newMode = static_cast<int>(std::round(p->get().value()));
        if (newMode != m_bandCountMode) {
            m_bandCountMode = std::clamp(newMode, 0, 2);
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
        buildBands(); // may update m_fastFftN
    }

    // Fast hop derives from current FFT size and overlap factor — recompute after buildBands.
    static constexpr int overlapFactors[] = { 32, 16, 8 }; // Fast/Normal/Slow
    const int newHopSize = m_fastFftN / overlapFactors[m_fftRateMode];
    if (newHopSize != m_fastHopSize) {
        m_fastHopSize = newHopSize;
        m_fastHopFill = 0;
        std::fill(m_fastInBuf.data(), m_fastInBuf.data() + m_fastFftN, 0.0);
    }
}

} // namespace noteahead
