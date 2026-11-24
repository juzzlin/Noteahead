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

#ifndef TRACKING_BELL_BANK_HPP
#define TRACKING_BELL_BANK_HPP

#include "spectral_peak_detector.hpp"
#include "svf_filter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace noteahead {

//! A set of cutting bells that follow the peaks a SpectralPeakDetector finds. A bell keeps to its
//! peak from one analysis to the next, gliding after it as it moves, so that a peak does not hop
//! from bell to bell; its gain moves with the attack while the peak asks for more and with the
//! release otherwise, and once it is back at 0 dB the bell is free for the next peak.
//!
//! Runs in the time domain, so it adds no latency. A bell at 0 dB passes its input untouched.
class TrackingBellBank
{
public:
    static constexpr size_t MaxBells { SpectralPeakDetector::MaxPeaks };
    //! The paths one bell's settings drive: two for linked stereo, one bell filter for each side.
    static constexpr size_t MaxPaths { 2 };

    struct Bell
    {
        double frequencyHz = 0.0;
        double gainDb = 0.0;
    };

    void setSampleRate(double sampleRate);
    //! How many bells may be cutting at once, at most MaxBells. Those over the count release.
    void setBellCount(size_t count);
    void setQ(double q);
    void setAttackMs(double attackMs);
    void setReleaseMs(double releaseMs);

    //! The detector's latest peaks, strongest first.
    void track(std::span<const SpectralPeakDetector::Peak> peaks);
    //! Once a frame, before process(): moves the bells every few frames.
    void advance();
    double process(size_t path, double input);

    //! The bells that are cutting, into @p bells; returns how many.
    size_t activeBells(std::span<Bell, MaxBells> bells) const;
    //! No bell is cutting.
    bool isSettled() const;
    double q() const;

    void reset();

private:
    void updateCoefficients();
    void updateFilters(size_t index);

    struct Slot
    {
        bool active = false;
        bool matched = false;
        double frequencyHz = 1000.0;
        double targetFrequencyHz = 1000.0;
        double gainDb = 0.0;
        double targetGainDb = 0.0;
        std::array<SvfFilter, MaxPaths> filters;
    };

    std::array<Slot, MaxBells> m_slots;
    double m_sampleRate { 48000.0 };
    size_t m_bellCount { 12 };
    double m_q { 4.0 };
    double m_attackMs { 10.0 };
    double m_releaseMs { 150.0 };
    double m_attackCoefficient { 0.0 };
    double m_releaseCoefficient { 0.0 };
    double m_glideCoefficient { 0.0 };
    uint32_t m_framesSinceUpdate { 0 };
};

} // namespace noteahead

#endif // TRACKING_BELL_BANK_HPP
