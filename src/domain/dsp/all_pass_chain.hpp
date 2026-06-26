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

#ifndef ALL_PASS_CHAIN_HPP
#define ALL_PASS_CHAIN_HPP

#include "dsp_component.hpp"

#include <array>

namespace noteahead {

// Chain of first-order all-pass sections y[n] = a*(x[n] - y[n-1]) + x[n-1].
// Used inside the Karplus-Strong feedback loop to model string dispersion (inharmonicity).
// Setting coefficient to 0 makes each stage a pure 1-sample delay with no spectral effect.
class AllPassChain : public DspComponent
{
public:
    static constexpr int MaxStages = 8;

    void setCoefficient(double coeff);
    void setStages(int stages);
    double process(double input);
    void reset();

private:
    std::array<double, MaxStages> m_x1 {};
    std::array<double, MaxStages> m_y1 {};
    int m_stages { 4 };
    double m_coeff { 0.0 };
};

} // namespace noteahead

#endif // ALL_PASS_CHAIN_HPP
