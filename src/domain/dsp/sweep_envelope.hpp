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

#ifndef SWEEP_ENVELOPE_HPP
#define SWEEP_ENVELOPE_HPP

#include "dsp_component.hpp"

namespace noteahead {

//! A curved ramp from a start value to an end value over a length in seconds or in beats.
//!
//! In beats the progress is advanced by the current tempo on every sample, so a tempo change in the
//! middle of a sweep applies to what is left of it and the sweep still ends on the beat it was
//! aimed at.
class SweepEnvelope : public DspComponent
{
public:
    //! Append only: persisted as the ordinal.
    enum class Mode
    {
        //! Stays at the end value once there.
        Hold,
        //! Jumps back to the start value and sweeps again.
        Loop,
        //! Sweeps back and forth between the start and end values.
        PingPong
    };

    void setSampleRate(double sampleRate) override;

    void setStartValue(double value);
    double startValue() const;
    void setEndValue(double value);
    double endValue() const;

    //! -1..1. Zero is linear, positive starts slowly and accelerates (a riser), negative moves
    //! quickly first and settles (a fall).
    void setCurve(double curve);
    double curve() const;

    void setMode(Mode mode);
    Mode mode() const;

    void setLengthSeconds(double seconds);
    void setLengthBeats(double beats);
    void setBpm(double bpm);

    //! Restarts the sweep from the start value.
    void trigger();

    //! The value at the current position, then advances by one sample.
    double nextSample();

    //! True from trigger() until a Hold sweep reaches its end; always true for the repeating modes
    //! once triggered.
    bool isRunning() const;

    //! Position along the sweep, 0..1, before the curve.
    double progress() const;

private:
    void updateStep();
    double shaped(double progress) const;

    double m_startValue { 0.0 };
    double m_endValue { 1.0 };
    double m_curve { 0.0 };
    Mode m_mode { Mode::Hold };

    bool m_synced { false };
    double m_lengthSeconds { 1.0 };
    double m_lengthBeats { 4.0 };
    double m_bpm { 120.0 };

    double m_step { 0.0 };
    double m_progress { 0.0 };
    double m_direction { 1.0 };
    bool m_running { false };
};

} // namespace noteahead

#endif // SWEEP_ENVELOPE_HPP
