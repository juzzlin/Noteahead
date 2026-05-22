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

#ifndef BIQUAD_FILTER_HPP
#define BIQUAD_FILTER_HPP

namespace noteahead {

class BiquadFilter
{
public:
    enum class Type
    {
        Bypass,
        Bell,
        LowShelf,
        HighShelf,
        LowCut,
        HighCut,
        Notch
    };

    void calculateBell(double frequency, double sampleRate, double q, double gainDb);
    void calculateLowShelf(double frequency, double sampleRate, double q, double gainDb);
    void calculateHighShelf(double frequency, double sampleRate, double q, double gainDb);
    void calculateLowCut(double frequency, double sampleRate, double q);
    void calculateHighCut(double frequency, double sampleRate, double q);
    void calculateNotch(double frequency, double sampleRate, double q);

    void setBypass();

    float process(float input);
    void reset();

private:
    struct Coefficients
    {
        double a1 { 0.0 };
        double a2 { 0.0 };
        double b0 { 1.0 };
        double b1 { 0.0 };
        double b2 { 0.0 };
    };

    Coefficients m_coefficients;
    double m_z1 { 0.0 };
    double m_z2 { 0.0 };
    bool m_isBypassed { true };
};

} // namespace noteahead

#endif // BIQUAD_FILTER_HPP
