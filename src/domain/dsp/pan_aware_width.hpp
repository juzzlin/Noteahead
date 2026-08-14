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

#ifndef PAN_AWARE_WIDTH_HPP
#define PAN_AWARE_WIDTH_HPP

#include <cstdint>
#include <optional>
#include <span>

namespace noteahead {

//! Narrows an already panned stereo signal towards the position it was panned to.
//!
//! Plain mid/side width collapses towards the centre, so it would undo a pan applied before it:
//! a hard-left sound at zero width would come out of the middle. This folds the signal along the
//! direction of the constant-power pan law instead, so zero width is mono at the pan position and
//! at the centre it is exactly mid/side width.
class PanAwareWidth
{
public:
    //! Processes @p frameCount interleaved stereo frames of @p buffer in place. @p width and @p pan
    //! are 0..1. A change from the previous block is ramped over this one. Full width with full
    //! width before it is left untouched, bit for bit.
    void process(std::span<double> buffer, uint32_t frameCount, double width, double pan);

    //! Forgets the previous block, so the next one starts at its own settings.
    void reset();

private:
    std::optional<double> m_width;
    double m_leftGain { 0.0 };
    double m_rightGain { 0.0 };
};

} // namespace noteahead

#endif // PAN_AWARE_WIDTH_HPP
