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

#ifndef KICK_SYNTH_PREVIEW_HPP
#define KICK_SYNTH_PREVIEW_HPP

#include <QVariantList>

namespace noteahead {

class KickSynthDevice;

//! Draws one hit of the Kick Synth, for the waveform view.
//!
//! As DrumVoicePreview draws a drum machine's voice: rendered on a copy of the device, made through
//! the format it is saved in, so that striking it does not tread on the one in the rack.
namespace KickSynthPreview {

struct Preview
{
    //! One peak per point asked for, 0..1.
    QVariantList peaks;
    //! What those peaks span, in seconds: the drum with its amp envelope left out, for the envelope
    //! to be drawn over rather than counted twice.
    double durationSeconds {};
    //! What is actually heard, in seconds: the drum with its envelope as it is set.
    double audibleSeconds {};
};

//! \param peakCount How many points the view has room for. Zero or fewer renders nothing.
Preview render(const KickSynthDevice & device, int peakCount, double sampleRate);

} // namespace KickSynthPreview

} // namespace noteahead

#endif // KICK_SYNTH_PREVIEW_HPP
