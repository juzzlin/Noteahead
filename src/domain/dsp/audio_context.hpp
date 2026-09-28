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

#ifndef AUDIO_CONTEXT_HPP
#define AUDIO_CONTEXT_HPP

#include <cstdint>
#include <span>

namespace noteahead {

/**
 * @brief Audio processing context.
 *
 * Uses double precision for the accumulation buffer to ensure high-quality
 * mixing and summing across many voices and tracks.
 */
struct AudioContext
{
    std::span<double> buffer {};
    uint32_t frameCount { 0 };
    uint32_t sampleRate { 0 };
    double bpm { 120.0 };
    std::span<const std::span<const double>> deviceOutputBuffers {};
    //! Internal oversampling factor (1, 2 or 4) for devices that render their nonlinear stages at a
    //! higher rate. Lower for realtime playback to save CPU, higher for offline export. Default 2
    //! preserves the historical fixed 2x behaviour.
    uint8_t oversampleFactor { 2 };
    //! Whether this block is being rendered offline rather than heard. Set by the offline export and
    //! carried into every derived context, so that an effect which only makes sense while listening --
    //! Monitor -- can take itself out of the signal path of an export without anything having to
    //! remember to reach it.
    bool offline { false };
    //! Where this block starts on the engine's stream timeline, in frames since the stream opened.
    //!
    //! What lets an event be placed at the frame it was written for rather than at whichever block
    //! boundary happens to come next: the engine breaks a block at each scheduled event and hands
    //! the pieces over with this moved along, so a device needs to know nothing about it.
    //!
    //! Last on purpose: the engine builds its context positionally, so a field added before one of
    //! those would silently take another's argument.
    uint64_t startFrame { 0 };

    //! The send buses this block is being mixed into, one buffer per bus, interleaved as @ref buffer
    //! is. Empty when the song has no send effects, and empty in any context derived for a part of a
    //! device rather than the device itself.
    //!
    //! What lets a device route parts of itself rather than only its whole output: a Sampler pad or
    //! a Drum Synth voice adds its own signal here, and the engine adds the device's own send on top
    //! afterwards, so the two taps are independent. Everything else leaves this alone.
    //!
    //! The buffers belong to the lane the device is being processed on, so writing here races
    //! nothing even while devices run in parallel.
    std::span<const std::span<double>> sendBuses {};

    //! Where this context's audio starts inside the block's send buses, in samples.
    //!
    //! Non-zero only when a block has been cut at a scheduled event: the piece's own buffer is a
    //! sub-span starting there, while the send buses stay whole. A device adding to a bus has to
    //! offset by this or a note landing mid-block would send the start of the block instead.
    size_t sendBusOffset { 0 };
};

} // namespace noteahead

#endif // AUDIO_CONTEXT_HPP
