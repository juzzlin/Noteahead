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

#ifndef DRUM_VOICE_PREVIEW_HPP
#define DRUM_VOICE_PREVIEW_HPP

#include <QVariantList>

namespace noteahead {

class DrumSynthV2Device;

//! Draws one drum voice, for the waveform view.
//!
//! A Sampler pad has a file to picture; a drum voice has nothing but the engine that makes it, so
//! the picture has to be rendered. It is rendered on a *copy* of the device rather than on the one
//! in the rack: rendering strikes the voice and runs the device forward, which on a live device
//! would tread on whatever the song is playing.
//!
//! Lives here rather than in the domain because copying a device means serializing it, and that is
//! infra: a domain device may not reach for it.
namespace DrumVoicePreview {

struct Preview
{
    //! One peak per point asked for, 0..1.
    QVariantList peaks;
    //! What those peaks span, in seconds: the voice with its amp envelope switched off.
    //!
    //! The envelope is drawn over the waveform rather than baked into it, as the Sampler draws one
    //! over a file it has not touched. Baking it in would count it twice -- once in the picture and
    //! once in the overlay -- and would hide the very thing the overlay is there to show, which is
    //! how much of the drum the envelope is cutting off.
    double durationSeconds {};
    //! What is actually heard, in seconds: the same voice with its envelope as it is set, and with
    //! the tail of whatever effects it runs through. This is the figure the readout quotes.
    double audibleSeconds {};
};

//! \param peakCount How many points the view has room for. Zero or fewer renders nothing.
Preview render(const DrumSynthV2Device & device, int voiceIndex, int peakCount, double sampleRate);

} // namespace DrumVoicePreview

} // namespace noteahead

#endif // DRUM_VOICE_PREVIEW_HPP
