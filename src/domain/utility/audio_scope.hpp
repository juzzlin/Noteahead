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

#ifndef AUDIO_SCOPE_HPP
#define AUDIO_SCOPE_HPP

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace noteahead {

//! Reusable stereo oscilloscope capture buffer.
//!
//! The audio thread pushes interleaved stereo output via write(); the UI thread polls a
//! trigger-aligned stereo waveform via snapshot(). Mirrors the Rta threading model: an atomic
//! gate makes write() a cheap no-op while no scope is displayed. The audio thread never blocks on
//! the mutex — write() try-locks and skips the buffer on contention — while snapshot() holds the
//! lock only long enough to copy the raw rings, doing the reordering/decimation unlocked.
class AudioScope
{
public:
    AudioScope();

    //! A trigger-aligned window of both channels. Left and right share the same start offset so
    //! the two traces stay phase-aligned relative to each other.
    struct Snapshot
    {
        std::vector<float> left;
        std::vector<float> right;
    };

    void setActive(bool active);
    bool active() const;

    //! Audio-thread: push interleaved stereo into the L/R rings. No-op when inactive.
    void write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate);

    //! UI-thread: return the latest window of both channels aligned to a rising zero-crossing of
    //! the left channel, decimated to at most maxPoints samples per channel (<= 0 means none).
    //! Values are raw output amplitudes.
    //!
    //! \param cycles When positive, the window is that many periods of whatever pitch the left
    //! channel is holding, so the trace stands still and shows the same number of cycles whatever
    //! note is played. Falls back to the fixed window when no pitch can be found -- on noise, on a
    //! chord, or on silence -- since a period that is not there cannot be locked to.
    Snapshot snapshot(size_t maxPoints, int cycles = 0) const;

    //! Pitch the last cycle-locked snapshot() found, in Hz, or 0 when it found none. A plain read
    //! of what that call already worked out: searching for the period again here would double the
    //! cost of the one genuinely expensive thing the scope does.
    double lastDetectedFrequency() const;

    uint32_t sampleRate() const;

private:
    static constexpr size_t ringSize = 4096;
    static constexpr size_t displayLength = 2048;

    std::atomic<bool> m_active { false };
    std::atomic<uint32_t> m_sampleRate { 0 };
    //! Period in samples the last locked snapshot found, 0 for none.
    mutable std::atomic<double> m_lastPeriod { 0.0 };

    //! Chronological copy of the left ring, newest last, plus the period search over it.
    static std::optional<double> findPeriod(const std::vector<float> & linear, uint32_t sampleRate);

    mutable std::mutex m_mutex;
    std::vector<float> m_ringL;
    std::vector<float> m_ringR;
    size_t m_writePos = 0;
};

} // namespace noteahead

#endif // AUDIO_SCOPE_HPP
