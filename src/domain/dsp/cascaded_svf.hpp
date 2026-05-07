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

#ifndef CASCADED_SVF_HPP
#define CASCADED_SVF_HPP

#include <cstdint>

#include "dsp_component.hpp"

namespace noteahead {

class CascadedSvf : public DspComponent
{
public:
    enum class Mode
    {
        LowPass,
        HighPass
    };

    void setCutoff(double cutoff); // 0.0 to 1.0
    void setResonance(double resonance); // 0.0 to 1.0
    void setMode(Mode mode);
    
    float process(float input);
    void reset();

private:
    double m_cutoff { 1.0 };
    double m_resonance { 0.0 };
    Mode m_mode { Mode::LowPass };

    struct SvfUnit {
        double s1 = 0.0, s2 = 0.0;
        float process(float input, double g, double damping, double k, Mode mode);
        void reset() { s1 = s2 = 0.0; }
    };

    SvfUnit m_unit1;
    SvfUnit m_unit2;
};

} // namespace noteahead

#endif // CASCADED_SVF_HPP
