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

#ifndef DC_BLOCKER_HPP
#define DC_BLOCKER_HPP

#include "dsp_component.hpp"

namespace noteahead {

// 1-pole IIR high-pass filter (y[n] = x[n] - x[n-1] + R*y[n-1]) tuned to ~5 Hz,
// used to remove DC offset introduced by nonlinear filters or oscillator discretization.
class DcBlocker : public DspComponent
{
public:
    double process(double input);
    void reset();

private:
    void updateCoefficient();

    double m_x1 { 0.0 };
    double m_y1 { 0.0 };
    double m_coeff { 0.9993 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // DC_BLOCKER_HPP
