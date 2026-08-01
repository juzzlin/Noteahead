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

#ifndef CLIP_DETECTOR_HPP
#define CLIP_DETECTOR_HPP

#include <atomic>
#include <cstdint>

namespace noteahead {

//! Sticky full-scale detector for a device's output.
//!
//! Unlike the level meters this is deliberately *not* gated by an active flag: the point of a clip
//! indicator is to still be lit when you come back and look at it, so it has to catch clipping that
//! happened while nothing was on screen. The cost is one comparison per sample, with an early exit
//! once the buffer is known to have clipped.
//!
//! Latches until clear() — falling back on its own would hide exactly the short overshoot the
//! indicator exists to report.
class ClipDetector
{
public:
    //! Level counted as clipping. Full scale: the engine sums in double and only the conversion to
    //! the output device actually truncates, so anything at or past 1.0 is what will be lost.
    static constexpr double Threshold { 1.0 };

    //! Audio-thread: scan one interleaved stereo buffer and latch if any sample reaches full scale.
    void write(const double * interleavedStereo, uint32_t frameCount);

    //! UI-thread.
    bool clipped() const;
    void clear();

private:
    std::atomic<bool> m_clipped { false };
};

} // namespace noteahead

#endif // CLIP_DETECTOR_HPP
