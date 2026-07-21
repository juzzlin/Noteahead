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
#include <vector>

namespace noteahead {

//! Reusable stereo oscilloscope capture buffer.
//!
//! The audio thread pushes interleaved stereo output via write(); the UI thread polls a
//! trigger-aligned stereo waveform via snapshot(). Mirrors the Rta threading model: an atomic
//! gate makes write() a cheap no-op while no scope is displayed, and a short-held mutex guards
//! the shared ring buffers on both sides.
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
    Snapshot snapshot(size_t maxPoints) const;

    uint32_t sampleRate() const;

private:
    static constexpr size_t ringSize = 4096;
    static constexpr size_t displayLength = 2048;

    std::atomic<bool> m_active { false };
    std::atomic<uint32_t> m_sampleRate { 0 };

    mutable std::mutex m_mutex;
    std::vector<float> m_ringL;
    std::vector<float> m_ringR;
    size_t m_writePos = 0;
};

} // namespace noteahead

#endif // AUDIO_SCOPE_HPP
