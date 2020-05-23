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

#ifndef DIODE_LADDER_FILTER_HPP
#define DIODE_LADDER_FILTER_HPP

#include "dsp_component.hpp"

namespace noteahead {

/**
 * @brief Zero-Delay Feedback Diode Ladder Filter (TB-303 style)
 *
 * A 4-pole lowpass filter with internal non-linearities and a feedback loop
 * that models the unique characteristics of a diode ladder.
 */
class DiodeLadderFilter : public DspComponent
{
public:
    void setCutoff(double cutoff);     // 0.0 to 1.0
    void setResonance(double resonance); // 0.0 to 1.0
    void setDrive(double drive);       // 0.0 to 1.0

    float process(float input);
    void reset();

private:
    double m_cutoff { 1.0 };
    double m_resonance { 0.0 };
    double m_drive { 0.0 };

    double m_lastCutoff { -1.0 };
    double m_lastResonance { -1.0 };
    double m_lastSampleRate { -1.0 };

    double m_g { 0.0 };
    double m_k { 0.0 };

    // State variables
    double m_s1 { 0.0 };
    double m_s2 { 0.0 };
    double m_s3 { 0.0 };
    double m_s4 { 0.0 };

    void updateCoefficients();
};

} // namespace noteahead

#endif // DIODE_LADDER_FILTER_HPP
