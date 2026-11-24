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

#include "tracking_bell_bank.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace noteahead {

namespace {

//! The bells move this often. A step of 16 frames is far finer than the fastest attack.
constexpr uint32_t UpdateInterval { 16 };
//! A peak this close to where a bell is going is that bell's peak.
constexpr double MatchOctaves { 0.25 };
//! How fast a bell glides after its peak: a time constant.
constexpr double GlideSeconds { 0.02 };
//! A bell this close to 0 dB, and asked for nothing, is free again.
constexpr double FreeBelowDb { 0.01 };

double coefficientFor(double seconds, double sampleRate)
{
    return std::exp(-static_cast<double>(UpdateInterval) / (std::max(1.0e-4, seconds) * sampleRate));
}

} // namespace

void TrackingBellBank::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    updateCoefficients();
}

void TrackingBellBank::setBellCount(size_t count)
{
    m_bellCount = std::clamp<size_t>(count, 1, MaxBells);
}

void TrackingBellBank::setQ(double q)
{
    if (q != m_q) {
        m_q = q;
        for (size_t i = 0; i < MaxBells; i++) {
            if (m_slots[i].active) {
                updateFilters(i);
            }
        }
    }
}

void TrackingBellBank::setAttackMs(double attackMs)
{
    m_attackMs = attackMs;
    updateCoefficients();
}

void TrackingBellBank::setReleaseMs(double releaseMs)
{
    m_releaseMs = releaseMs;
    updateCoefficients();
}

void TrackingBellBank::updateCoefficients()
{
    m_attackCoefficient = coefficientFor(m_attackMs / 1000.0, m_sampleRate);
    m_releaseCoefficient = coefficientFor(m_releaseMs / 1000.0, m_sampleRate);
    m_glideCoefficient = coefficientFor(GlideSeconds, m_sampleRate);
}

void TrackingBellBank::track(std::span<const SpectralPeakDetector::Peak> peaks)
{
    for (auto && slot : m_slots) {
        slot.matched = false;
    }
    const size_t peakCount = std::min(peaks.size(), m_bellCount);
    for (size_t p = 0; p < peakCount; p++) {
        const auto & peak = peaks[p];
        // The bell already on this peak, if there is one...
        std::optional<size_t> nearest;
        double nearestOctaves = MatchOctaves;
        for (size_t i = 0; i < MaxBells; i++) {
            const auto & slot = m_slots[i];
            if (slot.active && !slot.matched) {
                if (const double octaves = std::abs(std::log2(slot.targetFrequencyHz / peak.frequencyHz)); octaves < nearestOctaves) {
                    nearestOctaves = octaves;
                    nearest = i;
                }
            }
        }
        // ...or a free one, which starts at 0 dB where the peak is and so can be put there at once
        if (!nearest) {
            size_t activeCount = 0;
            for (auto && slot : m_slots) {
                activeCount += slot.active ? 1 : 0;
            }
            if (activeCount >= m_bellCount) {
                continue;
            }
            for (size_t i = 0; i < MaxBells; i++) {
                if (auto && slot = m_slots[i]; !slot.active) {
                    slot.active = true;
                    slot.frequencyHz = peak.frequencyHz;
                    slot.gainDb = 0.0;
                    for (auto && filter : slot.filters) {
                        filter.reset();
                    }
                    updateFilters(i);
                    nearest = i;
                    break;
                }
            }
        }
        if (nearest) {
            auto && slot = m_slots[*nearest];
            slot.matched = true;
            slot.targetFrequencyHz = peak.frequencyHz;
            slot.targetGainDb = -peak.reductionDb;
        }
    }
    // A bell whose peak has gone lets go
    for (auto && slot : m_slots) {
        if (slot.active && !slot.matched) {
            slot.targetGainDb = 0.0;
        }
    }
}

void TrackingBellBank::advance()
{
    if (++m_framesSinceUpdate < UpdateInterval) {
        return;
    }
    m_framesSinceUpdate = 0;
    for (size_t i = 0; i < MaxBells; i++) {
        auto && slot = m_slots[i];
        if (!slot.active) {
            continue;
        }
        const double coefficient = slot.targetGainDb < slot.gainDb ? m_attackCoefficient : m_releaseCoefficient;
        slot.gainDb = slot.targetGainDb + (slot.gainDb - slot.targetGainDb) * coefficient;
        slot.frequencyHz *= std::pow(slot.targetFrequencyHz / slot.frequencyHz, 1.0 - m_glideCoefficient);
        if (!slot.matched && slot.targetGainDb == 0.0 && std::abs(slot.gainDb) < FreeBelowDb) {
            slot.active = false;
            slot.gainDb = 0.0;
            continue;
        }
        updateFilters(i);
    }
}

void TrackingBellBank::updateFilters(size_t index)
{
    auto && slot = m_slots[index];
    const double frequencyHz = std::clamp(slot.frequencyHz, 10.0, m_sampleRate * 0.45);
    for (auto && filter : slot.filters) {
        filter.calculateBell(frequencyHz, m_sampleRate, m_q, slot.gainDb);
    }
}

double TrackingBellBank::process(size_t path, double input)
{
    double output = input;
    for (auto && slot : m_slots) {
        if (slot.active) {
            output = slot.filters[path].process(output);
        }
    }
    return output;
}

size_t TrackingBellBank::activeBells(std::span<Bell, MaxBells> bells) const
{
    size_t count = 0;
    for (auto && slot : m_slots) {
        if (slot.active) {
            bells[count++] = { slot.frequencyHz, slot.gainDb };
        }
    }
    return count;
}

bool TrackingBellBank::isSettled() const
{
    return std::none_of(m_slots.begin(), m_slots.end(), [](const Slot & slot) {
        return slot.active;
    });
}

double TrackingBellBank::q() const
{
    return m_q;
}

void TrackingBellBank::reset()
{
    for (auto && slot : m_slots) {
        slot.active = false;
        slot.matched = false;
        slot.gainDb = 0.0;
        slot.targetGainDb = 0.0;
        for (auto && filter : slot.filters) {
            filter.reset();
        }
    }
    m_framesSinceUpdate = 0;
}

} // namespace noteahead
