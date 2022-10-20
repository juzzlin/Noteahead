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

#ifndef LOAD_METER_HPP
#define LOAD_METER_HPP

#include <atomic>
#include <chrono>
#include <cstdint>

namespace noteahead {

//! How much of an audio buffer's real-time budget something took.
//!
//! 100% means it used exactly as long to produce a buffer as that buffer takes to play, which is the
//! point where audio starts breaking up. Mirrors the AudioScope and LevelMeter threading model — an
//! atomic gate makes the measurement a no-op while nothing is displayed — and like LevelMeter it
//! needs no mutex: the audio thread only stores, the reader only loads.
class LoadMeter
{
public:
    void setActive(bool active);
    bool active() const;

    //! Audio-thread: record how long a buffer took against how long it lasts. No-op when inactive.
    void addBlock(std::chrono::nanoseconds elapsed, double bufferSeconds);

    //! Smoothed load, in percent of the real-time budget.
    float loadPercent() const;
    //! Worst load seen recently, in percent. Falls back so a single spike does not stick forever.
    float peakPercent() const;

    //! Buffers that took longer than they last. Only meaningful for the engine-wide meter, where it
    //! counts what the listener hears as a dropout.
    uint64_t overrunCount() const;

    void reset();

private:
    //! Averaging window, in seconds of audio. Long enough to read, short enough to react.
    static constexpr float WindowSeconds { 0.5f };
    //! How fast the peak reading falls, in percentage points per second.
    static constexpr float PeakFallbackPerSecond { 50.0f };

    std::atomic<bool> m_active { false };
    std::atomic<float> m_load { 0.0f };
    std::atomic<float> m_peak { 0.0f };
    std::atomic<uint64_t> m_overruns { 0 };
};

} // namespace noteahead

#endif // LOAD_METER_HPP
