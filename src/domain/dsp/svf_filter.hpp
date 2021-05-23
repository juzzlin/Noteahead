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

#ifndef SVF_FILTER_HPP
#define SVF_FILTER_HPP

namespace noteahead {

class SvfFilter
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

    double process(double input);
    void reset();

private:
    double m_g { 0.0 };
    double m_k { 0.0 };
    double m_a1 { 0.0 };
    double m_a2 { 0.0 };
    double m_a3 { 0.0 };
    double m_m0 { 0.0 };
    double m_m1 { 0.0 };
    double m_m2 { 0.0 };

    double m_s1 { 0.0 };
    double m_s2 { 0.0 };

    bool m_isBypassed { true };
};

} // namespace noteahead

#endif // SVF_FILTER_HPP
