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

#ifndef MICRO_PITCH_SHIFTER_HPP
#define MICRO_PITCH_SHIFTER_HPP

#include "delay_line.hpp"
#include "dsp_component.hpp"

namespace noteahead {

//! Shifts pitch by a few cents, by reading a delay line faster or slower than it is written.
//!
//! A read pointer that closes on the write pointer by a constant fraction of a sample per sample
//! returns the signal sped up by that fraction, which is a pitch shift. It cannot do so for long:
//! the pointer eventually reaches the end of the window it has to work in and has to jump back,
//! and the jump is a discontinuity. A second tap a whole window away covers it -- the pointer lands
//! exactly where that tap already was -- and the two are crossfaded across the handover.
//!
//! The handover is kept short deliberately. Two taps of one signal at different delays comb against
//! each other, and at some frequencies cancel outright, so they are summed for a few milliseconds
//! at each jump rather than continuously.
//!
//! Sized for the few cents a widener or a doubler wants rather than for transposition. At that
//! range the pointer creeps: a seven cent shift crosses a fifteen millisecond window about once
//! every four seconds, so the handover runs for about two thousandths of the time.
class MicroPitchShifter : public DspComponent
{
public:
    void setSampleRate(double sampleRate) override;

    //! Shift in cents, positive up. Small values are what this is for.
    void setCents(double cents);
    double cents() const;

    double process(double input);

    void reset();

private:
    void update();

    DelayLine m_delayLine;

    double m_cents { 0.0 };
    double m_windowSamples { 0.0 };
    double m_crossfadeSamples { 0.0 };
    //! Where the leading tap reads, in samples behind the write pointer. Travels between one and
    //! two windows, which is also the shifted signal's own delay.
    double m_delay { 0.0 };
    //! What that delay changes by each sample, which is what sets the shift.
    double m_rate { 0.0 };

    double m_lastSampleRate { -1.0 };
    bool m_dirty { true };
};

} // namespace noteahead

#endif // MICRO_PITCH_SHIFTER_HPP
