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

#ifndef DRUM_HIT_PREVIEW_HPP
#define DRUM_HIT_PREVIEW_HPP

#include <QVariantList>

#include <cstdint>
#include <functional>
#include <memory>

namespace noteahead {

class Device;

//! Draws one hit of a single drum, such as Kick 808 or Kick 909, for the waveform view.
//!
//! As DrumVoicePreview draws a drum machine's voice: rendered on a copy of the device, made through
//! the format it is saved in, so that striking it does not tread on the one in the rack.
namespace DrumHitPreview {

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

//! A new device of the same kind, for the copy to be read into.
using CopyFactory = std::function<std::unique_ptr<Device>()>;
//! Sets the copy's amp envelope to do nothing.
using EnvelopeBypass = std::function<void(Device &)>;

//! \param note The note the hit is struck with.
//! \param peakCount How many points the view has room for. Zero or fewer renders nothing.
Preview render(const Device & device, const CopyFactory & createCopy, const EnvelopeBypass & bypassEnvelope, uint8_t note, int peakCount, double sampleRate);

} // namespace DrumHitPreview

} // namespace noteahead

#endif // DRUM_HIT_PREVIEW_HPP
