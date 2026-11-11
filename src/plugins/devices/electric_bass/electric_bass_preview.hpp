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

#ifndef ELECTRIC_BASS_PREVIEW_HPP
#define ELECTRIC_BASS_PREVIEW_HPP

#include <QVariantList>

namespace noteahead {

class ElectricBassDevice;

//! Draws one note of the Electric Bass, for the waveform view: the open E plucked at full
//! strength, held, and let go, so that the pluck, the sustain and the release all show.
//!
//! Rendered on a copy of the device, made through the format it is saved in, so that playing it
//! does not tread on the one in the rack.
namespace ElectricBassPreview {

struct Preview
{
    //! One peak per point asked for, 0..1.
    QVariantList peaks;
    //! What those peaks span, in seconds
    double durationSeconds {};
};

//! \param peakCount How many points the view has room for. Zero or fewer renders nothing.
Preview render(const ElectricBassDevice & device, int peakCount, double sampleRate);

} // namespace ElectricBassPreview

} // namespace noteahead

#endif // ELECTRIC_BASS_PREVIEW_HPP
