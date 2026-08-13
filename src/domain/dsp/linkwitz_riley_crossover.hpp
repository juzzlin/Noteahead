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

#ifndef LINKWITZ_RILEY_CROSSOVER_HPP
#define LINKWITZ_RILEY_CROSSOVER_HPP

#include "dsp_component.hpp"

namespace noteahead {

//! Fourth-order Linkwitz-Riley crossover: one mono input in, a low and a high band out, which sum
//! back to a flat magnitude response with neither band inverted.
//!
//! Each path is two cascaded Butterworth (Q = 1/sqrt(2)) TPT state variable sections. Squaring the
//! Butterworth response is what makes the pair sum to an all-pass instead of to a bump at the
//! corner, and it is why a band can be split and recombined without the crossover being audible.
class LinkwitzRileyCrossover : public DspComponent
{
public:
    //! Corner frequency in Hz. Clamped to the audio band and kept away from Nyquist.
    void setCutoff(double frequency);
    double cutoff() const;

    void process(double input, double & low, double & high);

    //! The all-pass this crossover amounts to, which is just its own two bands summed.
    //!
    //! A three-way split needs it: the low band leaves the first crossover before the second one
    //! exists, so it never picks up the second corner's phase rotation, and summing it back against
    //! bands that did would notch the response there. Running it through an instance parked on the
    //! second corner gives it that rotation and nothing else.
    double processAllPass(double input);

    void reset();

private:
    struct Section
    {
        double s1 { 0.0 };
        double s2 { 0.0 };
    };

    void updateCoefficients();
    double processLowPass(Section & section, double input) const;
    double processHighPass(Section & section, double input) const;

    double m_cutoff { 1000.0 };

    double m_g { 0.0 };
    double m_k { 0.0 };
    double m_den { 0.0 };

    double m_lastCutoff { -1.0 };
    double m_lastSampleRate { -1.0 };

    Section m_lowFirst;
    Section m_lowSecond;
    Section m_highFirst;
    Section m_highSecond;
};

} // namespace noteahead

#endif // LINKWITZ_RILEY_CROSSOVER_HPP
