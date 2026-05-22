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

#ifndef MATCHED_BELL_FILTER_HPP
#define MATCHED_BELL_FILTER_HPP

#include <complex>

namespace noteahead {

//! A peaking filter whose magnitude follows its analog prototype all the way up to Nyquist.
//!
//! The bilinear transform every other bell here is built on squeezes the top octave of the analog
//! response into what is left below Nyquist and pins the gain there to 0 dB, so a bell near the
//! top of the band is narrower than asked and falls away on its upper side. On its own that is
//! a detail; on a graphic equalizer whose top bands sit at 16 and 20 kHz it opens a dip of several
//! decibels between them that no choice of band gains can fill.
//!
//! The poles here are the analog ones mapped by impulse invariance, and the zeros are solved so
//! that the magnitude equals the analog one at DC, at the centre and at Nyquist (after Vicanek,
//! "Matched Second Order Digital Filters"). The prototype is the same constant-Q bell
//! SvfFilter::calculateBell() uses, so a cut is the exact inverse of the boost of the same size.
//! That is how a cut is built, too: matching a deep cut directly near Nyquist has no real
//! solution, while the boost always has one with its zeros inside the unit circle, so its inverse
//! is stable.
class MatchedBellFilter
{
public:
    void calculate(double frequency, double sampleRate, double q, double gainDb);
    void setBypass();
    bool isBypassed() const;

    //! The response of the coefficients process() runs on.
    std::complex<double> responseAt(double frequency, double sampleRate) const;
    double magnitudeDbAt(double frequency, double sampleRate) const;

    double process(double input);
    void reset();

private:
    void calculateBoost(double frequency, double sampleRate, double q, double gainDb);

    double m_b0 { 1.0 };
    double m_b1 { 0.0 };
    double m_b2 { 0.0 };
    double m_a1 { 0.0 };
    double m_a2 { 0.0 };

    double m_z1 { 0.0 };
    double m_z2 { 0.0 };

    bool m_isBypassed { true };
};

} // namespace noteahead

#endif // MATCHED_BELL_FILTER_HPP
