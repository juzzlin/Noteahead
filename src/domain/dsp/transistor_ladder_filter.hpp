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

#ifndef TRANSISTOR_LADDER_FILTER_HPP
#define TRANSISTOR_LADDER_FILTER_HPP

#include "dsp_component.hpp"

#include <array>

namespace noteahead {

//! The Minimoog's VCF: four transistor stages in a row, the output fed back to the input inverted.
//!
//! What sets it apart from the Juno's OTA ladder is what the transistors do with level. Every stage
//! is a differential pair, so every stage saturates, not only the input: a loud mixer is rounded off
//! on the way down the ladder as well as at its top, which is the Minimoog's overdrive. And nothing
//! gives back the passband the resonance takes away: at the edge of oscillation the bass is a fifth
//! of what it was, which is why Emphasis thins a Minimoog out.
//!
//! The loop is solved without a unit delay (Zavalishin's topology-preserving transform) on the
//! ladder's small-signal behaviour, so the corner and the self-oscillation pitch land where they are
//! asked to at any rate.
//!
//! The high-pass mode is the Behringer's: the same loop, heard through a mix of the stage outputs
//! that cancels the four low-passes into four high-passes, so it resonates as the low-pass does.
class TransistorLadderFilter : public DspComponent
{
public:
    enum class Mode
    {
        LowPass,
        HighPass
    };

    void setMode(Mode mode);
    Mode mode() const;

    //! \param frequency Corner in Hz.
    void setCutoff(double frequency);

    //! \param resonance 0 for none, 0.8 for the edge of self-oscillation, as Emphasis 8 is on a Minimoog.
    void setResonance(double resonance);

    //! \param input About 1 is where the transistors start to round the signal off.
    double process(double input);
    void reset();

    //! Feedback at the top of the Emphasis control. Four is where an ideal ladder starts to sing, and
    //! the transistors hold what is past it to a sine about a quarter of the ladder's full level.
    static constexpr double MaxFeedback { 5.0 };

private:
    Mode m_mode { Mode::LowPass };
    double m_cutoff { 1000.0 };
    double m_feedback { 0.0 };
    double m_lastCutoff { -1.0 };
    double m_lastSampleRate { -1.0 };
    double m_g { 0.0 };
    double m_stageGain { 0.0 };
    std::array<double, 4> m_state {};
};

} // namespace noteahead

#endif // TRANSISTOR_LADDER_FILTER_HPP
