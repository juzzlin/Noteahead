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

#ifndef POLY_BLEP_OSCILLATOR_HPP
#define POLY_BLEP_OSCILLATOR_HPP

#include "dsp_component.hpp"
#include "one_pole_filter.hpp"

#include <string>
#include <vector>

namespace noteahead {

class PolyBlepOscillator : public DspComponent
{
public:
    enum class Waveform
    {
        Triangle,
        Saw,
        Square,
        Sine
    };

    static std::vector<std::string> waveformNames();

    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);
    void setWaveform(Waveform waveform);
    void setPulseWidth(double pw); // 0.0 to 1.0
    void setShape(double shape); // 0.0 to 1.0

    double nextSample();
    void sync(double phase);

    //! Back to silence: phase to zero and the pulse stage's filters cleared. Hard sync deliberately
    //! does not do this -- a coupling capacitor does not empty itself because an oscillator was
    //! restarted mid-note -- so a voice being reset has to say so.
    void reset();

    double frequency() const;
    Waveform waveform() const;
    double pulseWidth() const;
    double shape() const;
    double phase() const;

private:
    double m_frequency { 440.0 };
    Waveform m_waveform { Waveform::Saw };
    double m_pulseWidth { 0.5 };
    double m_shape { 0.0 };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };

    double polyBlep(double t) const;
    void updatePhaseStep();

    //! The pulse, with the two things that make an analog one look and sound like it does: the
    //! supply is AC coupled, so the flat parts sag towards zero instead of sitting flat, and the
    //! edges take a finite time to get from one rail to the other.
    double analogPulse(double value);

    //! AC coupling. Its high-pass tap is the output: it removes the offset a duty cycle other than
    //! half leaves behind, and the sag on the way is the dip along the top of the wave.
    OnePoleFilter m_pulseCoupling;
    //! Finite edge rate. Rounds the corners, and takes the top off a narrow pulse that does not
    //! stay at the rail long enough to reach it -- which is what keeps a thin pulse nasal instead
    //! of piercing.
    OnePoleFilter m_pulseEdge;
};

} // namespace noteahead

#endif // POLY_BLEP_OSCILLATOR_HPP
