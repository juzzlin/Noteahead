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

#ifndef LEVEL_METER_HPP
#define LEVEL_METER_HPP

#include <atomic>
#include <cstdint>

namespace noteahead {

//! Peak and RMS level tap for gain staging.
//!
//! Mirrors the AudioScope threading model — an atomic gate makes write() a cheap no-op while no
//! meter is on screen — but needs no mutex at all: there is no ring buffer to protect, only two
//! scalars the audio thread stores and the UI thread loads.
//!
//! Peak falls back at a fixed rate rather than being reset by the reader, so the displayed value
//! does not depend on how often the UI happens to poll. RMS is smoothed over a fixed window for the
//! same reason.
class LevelMeter
{
public:
    //! Level reported when nothing is sounding. Also the floor of the dB conversion.
    static constexpr float MinimumDb { -120.0f };

    void setActive(bool active);
    bool active() const;

    //! Audio-thread: fold one interleaved stereo buffer into the running levels. No-op when inactive.
    void write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate);

    //! UI-thread: current levels in dBFS, clamped at MinimumDb.
    float peakDb() const;
    float rmsDb() const;

    void reset();

private:
    //! How fast the peak reading falls when the signal drops, in dB per second.
    static constexpr float PeakFallbackDbPerSecond { 20.0f };
    //! Averaging window of the RMS reading, in seconds.
    static constexpr float RmsWindowSeconds { 0.3f };

    std::atomic<bool> m_active { false };
    std::atomic<float> m_peak { 0.0f };
    std::atomic<float> m_meanSquare { 0.0f };
};

} // namespace noteahead

#endif // LEVEL_METER_HPP
