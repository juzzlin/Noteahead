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

#include "audio_scope.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

AudioScope::AudioScope()
  : m_ringL(ringSize, 0.0f)
  , m_ringR(ringSize, 0.0f)
{
}

void AudioScope::setActive(bool active)
{
    m_active.store(active);
}

bool AudioScope::active() const
{
    return m_active.load();
}

void AudioScope::write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate)
{
    if (!m_active.load()) {
        return;
    }

    // Never block the audio thread: if the UI thread is reading a snapshot, skip capturing this
    // buffer rather than wait on the lock. A missed buffer only causes a negligible visual gap,
    // whereas blocking here would stall the audio callback and cause stutter.
    std::unique_lock<std::mutex> lock { m_mutex, std::try_to_lock };
    if (!lock.owns_lock()) {
        return;
    }

    m_sampleRate.store(sampleRate);

    for (uint32_t i = 0; i < frameCount; i++) {
        m_ringL[m_writePos] = static_cast<float>(interleavedStereo[i * 2]);
        m_ringR[m_writePos] = static_cast<float>(interleavedStereo[i * 2 + 1]);
        m_writePos = (m_writePos + 1) % ringSize;
    }
}

namespace {

//! Pitch range the lock covers, as periods in samples. Wide enough for the bottom of a bass patch
//! and the top of a lead, and no wider: every extra lag is searched on the UI thread every frame.
constexpr double MinDetectableHz = 40.0;
constexpr double MaxDetectableHz = 4000.0;

//! How much of the ring the period search runs over. A whole ring would be more reliable and cost
//! four times as much; this is enough to hold several periods of anything in the range above.
constexpr size_t CorrelationWindow = 1024;

//! How well the best lag has to correlate before it is believed. Noise and chords sit well below
//! this, and locking to a period they do not have would leave the trace sliding about.
constexpr double CorrelationThreshold = 0.6;

} // namespace

std::optional<double> AudioScope::findPeriod(const std::vector<float> & linear, uint32_t sampleRate)
{
    if (!sampleRate || linear.size() < CorrelationWindow * 2) {
        return std::nullopt;
    }

    const auto minLag = static_cast<size_t>(std::floor(sampleRate / MaxDetectableHz));
    const auto maxLag = std::min(static_cast<size_t>(std::ceil(sampleRate / MinDetectableHz)), linear.size() - CorrelationWindow - 1);
    if (minLag < 2 || maxLag <= minLag) {
        return std::nullopt;
    }

    // The most recent CorrelationWindow samples, matched against the same length further back.
    const size_t end = linear.size();
    const size_t base = end - CorrelationWindow;

    double energy = 0.0;
    for (size_t i = 0; i < CorrelationWindow; i++) {
        const double sample = linear[base + i];
        energy += sample * sample;
    }
    if (energy < 1.0e-9) {
        return std::nullopt; // Silence has no period
    }

    double bestScore = 0.0;
    size_t bestLag = 0;
    for (size_t lag = minLag; lag <= maxLag; lag++) {
        if (base < lag) {
            break;
        }
        double correlation = 0.0;
        double laggedEnergy = 0.0;
        for (size_t i = 0; i < CorrelationWindow; i++) {
            const double current = linear[base + i];
            const double lagged = linear[base - lag + i];
            correlation += current * lagged;
            laggedEnergy += lagged * lagged;
        }
        if (laggedEnergy < 1.0e-9) {
            continue;
        }
        // Normalized, so a loud lag cannot beat a well-matching one on level alone.
        if (const double score = correlation / std::sqrt(energy * laggedEnergy); score > bestScore) {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestLag == 0 || bestScore < CorrelationThreshold) {
        return std::nullopt;
    }

    return static_cast<double>(bestLag);
}

double AudioScope::lastDetectedFrequency() const
{
    const double period = m_lastPeriod.load();
    const auto sampleRate = m_sampleRate.load();
    return period > 0.0 && sampleRate ? static_cast<double>(sampleRate) / period : 0.0;
}

AudioScope::Snapshot AudioScope::snapshot(size_t maxPoints, int cycles) const
{
    // Copy the raw rings under the lock, then release it before doing the (comparatively slow)
    // reordering and decimation. This keeps the critical section down to two plain buffer copies so
    // the audio thread's write() is almost never made to skip a buffer.
    std::vector<float> rawL(ringSize, 0.0f);
    std::vector<float> rawR(ringSize, 0.0f);
    size_t writePos = 0;
    {
        const std::lock_guard<std::mutex> lock { m_mutex };
        std::copy(m_ringL.begin(), m_ringL.end(), rawL.begin());
        std::copy(m_ringR.begin(), m_ringR.end(), rawR.begin());
        writePos = m_writePos;
    }

    // Reorder both rings into chronological linear buffers ending at the newest sample.
    std::vector<float> linearL(ringSize, 0.0f);
    std::vector<float> linearR(ringSize, 0.0f);
    for (size_t j = 0; j < ringSize; j++) {
        const size_t index = (writePos + j) % ringSize;
        linearL[j] = rawL[index];
        linearR[j] = rawR[index];
    }

    // How long a window to show. Locked to the pitch when asked for and when there is a pitch to
    // lock to, so the same number of cycles fills the width whatever note is playing; otherwise the
    // fixed window, which is what the scope has always shown.
    size_t windowLength = displayLength;
    if (cycles > 0) {
        const auto period = findPeriod(linearL, m_sampleRate.load());
        m_lastPeriod.store(period.value_or(0.0));
        if (period.has_value()) {
            const auto requested = static_cast<size_t>(std::llround(*period * cycles));
            windowLength = std::clamp(requested, size_t { 8 }, ringSize / 2);
        }
    } else {
        m_lastPeriod.store(0.0);
    }

    // Align the display window to the first rising zero-crossing of the left channel so the trace
    // is stable across frames; both channels share the same start to stay phase-aligned.
    const size_t searchEnd = ringSize - windowLength;
    size_t start = 0;
    for (size_t i = 0; i < searchEnd; i++) {
        if (linearL[i] <= 0.0f && linearL[i + 1] > 0.0f) {
            start = i;
            break;
        }
    }

    // Decimate each channel's window to at most maxPoints samples.
    const size_t count = (maxPoints > 0 && maxPoints < windowLength) ? maxPoints : windowLength;
    Snapshot result;
    result.left.reserve(count);
    result.right.reserve(count);
    for (size_t k = 0; k < count; k++) {
        const size_t index = start + (k * windowLength) / count;
        result.left.push_back(linearL[index]);
        result.right.push_back(linearR[index]);
    }
    return result;
}

uint32_t AudioScope::sampleRate() const
{
    return m_sampleRate.load();
}

} // namespace noteahead
