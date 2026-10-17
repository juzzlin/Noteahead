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

#ifndef JUNO_CHORUS_HPP
#define JUNO_CHORUS_HPP

#include "dsp_component.hpp"
#include "one_pole_filter.hpp"

#include <array>
#include <utility>
#include <vector>

namespace noteahead {

//! The Juno-60's chorus: one mono input into two bucket-brigade delays swept by a single triangle
//! LFO, the right-hand delay by the LFO inverted. Each output is the dry signal plus its own delay,
//! so the two sides move in opposite directions and the sound spreads without a fixed stereo image.
//!
//! The modes are the three buttons: I and II sweep the same delay range at different rates, and
//! I + II, both buttons down, is a fast and shallow wobble rather than a sum of the other two.
//! Rates and ranges are the commonly cited measurements of the Juno-60 circuit.
class JunoChorus : public DspComponent
{
public:
    enum class Mode
    {
        Off,
        I,
        II,
        Both
    };

    JunoChorus();

    void setSampleRate(double sampleRate) override;
    void setMode(Mode mode);
    Mode mode() const;

    //! One mono sample in, a stereo pair out. With the chorus off both sides are the input as is.
    void process(double input, double & left, double & right);
    void reset();

    //! The delay range and LFO rate a mode runs at.
    static double minDelayMs(Mode mode);
    static double maxDelayMs(Mode mode);
    static double rateHz(Mode mode);

    //! The left and right delays the last sample was read at, in milliseconds.
    std::pair<double, double> currentDelaysMs() const;

private:
    double readDelayed(double delaySamples) const;

    Mode m_mode { Mode::Off };
    std::vector<double> m_buffer;
    size_t m_writePos { 0 };
    double m_lfoPhase { 0.0 };
    std::pair<double, double> m_currentDelaysMs { 0.0, 0.0 };

    //! The bucket brigades' own anti-alias and reconstruction filters, which take the top off the
    //! delayed signal and nothing off the dry one. Two poles a side.
    std::array<OnePoleFilter, 2> m_lowPassL;
    std::array<OnePoleFilter, 2> m_lowPassR;
};

} // namespace noteahead

#endif // JUNO_CHORUS_HPP
