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

#include <complex>

namespace noteahead {

class SvfFilter
{
public:
    //! Append only. Eq8BandParametric persists a band's type as this enum's ordinal and casts the
    //! stored integer straight back, so reordering silently reinterprets every saved project.
    enum class Type
    {
        Bypass,
        Bell,
        LowShelf,
        HighShelf,
        LowCut,
        HighCut,
        Notch,
        BandPass
    };

    void calculateBell(double frequency, double sampleRate, double q, double gainDb);

    //! Unity-peak band-pass tap: passes the band around the corner and rejects everything else.
    //!
    //! Unlike the shaping calculators this one does not carry the dry signal, so it is the building
    //! block for parallel-summed equalizers, where band outputs are added to an untouched dry path.
    //! Note that setBypass() still returns the input unchanged, which is transparent for a shaping
    //! filter but not for a tap: drop the tap from the sum instead of bypassing it.
    void calculateBandPass(double frequency, double sampleRate, double q);

    void calculateLowShelf(double frequency, double sampleRate, double q, double gainDb);
    void calculateHighShelf(double frequency, double sampleRate, double q, double gainDb);
    void calculateLowCut(double frequency, double sampleRate, double q);
    void calculateHighCut(double frequency, double sampleRate, double q);
    void calculateNotch(double frequency, double sampleRate, double q);

    void setBypass();

    //! Magnitude of this filter at @p frequency, as a linear ratio.
    //!
    //! Derived from the very coefficients process() runs on rather than from a formula per type, so
    //! a curve drawn with it is the curve the audio takes -- including the frequency warping the
    //! bilinear transform puts in near Nyquist, which a drawing of the analog prototype would miss.
    double magnitudeAt(double frequency, double sampleRate) const;

    //! Complex response of this filter at @p frequency.
    //!
    //! Magnitude alone is enough for a cascade, where responses multiply and the phases come along
    //! for the ride. A parallel-summed equalizer needs this instead: its taps meet the dry path at
    //! different phases, and adding their magnitudes would draw a wider curve than it is heard at
    //! every frequency but a tap's own centre.
    std::complex<double> responseAt(double frequency, double sampleRate) const;

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
