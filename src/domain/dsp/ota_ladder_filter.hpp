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

#ifndef OTA_LADDER_FILTER_HPP
#define OTA_LADDER_FILTER_HPP

#include "dsp_component.hpp"

#include <array>

namespace noteahead {

//! The Juno's VCF: four one-pole stages in a row, the IR3109's OTAs, with the output fed back to
//! the input inverted. Unlike two resonant two-pole stages it has a single peak, sings on its own at
//! the top of the resonance control, and loses bass as the resonance comes up, because the feedback
//! subtracts the passband from the input. That loss is a large part of how a Juno squelches.
//!
//! The loop is solved without a unit delay (Zavalishin's topology-preserving transform), so the
//! corner and the self-oscillation pitch land where they are asked to at any rate. The OTAs'
//! saturation is a tanh where the feedback meets the input, which is what bounds the oscillation.
class OtaLadderFilter : public DspComponent
{
public:
    //! \param frequency Corner in Hz.
    void setCutoff(double frequency);
    //! \param resonance 0 for none, about 0.87 for the edge of self-oscillation, 1 for a steady sine.
    void setResonance(double resonance);
    double process(double input);
    void reset();

    //! Feedback at the top of the resonance control. Four is where an ideal ladder starts to sing.
    static constexpr double MaxFeedback { 4.2 };
    //! How much of the passband the resonance takes away is given back at the input. Most, not all:
    //! a Juno-106 recording loses about 1.2 dB of bass to a resonance that lifts the highs by 10 dB.
    static constexpr double BassCompensation { 0.8 };

private:
    double m_cutoff { 1000.0 };
    double m_feedback { 0.0 };
    double m_lastCutoff { -1.0 };
    double m_lastSampleRate { -1.0 };
    double m_g { 0.0 };
    double m_stageGain { 0.0 };
    std::array<double, 4> m_state {};
};

} // namespace noteahead

#endif // OTA_LADDER_FILTER_HPP
