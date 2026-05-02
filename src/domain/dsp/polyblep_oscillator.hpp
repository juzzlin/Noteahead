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

#ifndef POLYBLEP_OSCILLATOR_HPP
#define POLYBLEP_OSCILLATOR_HPP

namespace noteahead {

class PolyBLEPOscillator
{
public:
    enum class Waveform
    {
        Triangle,
        Saw,
        Pulse
    };

    void setSampleRate(double sampleRate);
    void setFrequency(double frequency);
    void setWaveform(Waveform waveform);
    void setPulseWidth(double pw); // 0.0 to 1.0
    void setShape(double shape); // 0.0 to 1.0

    double nextSample();
    void sync(double phase);

    double phase() const;

private:
    double m_sampleRate { 44100.0 };
    double m_frequency { 440.0 };
    Waveform m_waveform { Waveform::Saw };
    double m_pulseWidth { 0.5 };
    double m_shape { 0.0 };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };

    double polyBlep(double t) const;
    void updatePhaseStep();
};

} // namespace noteahead

#endif // POLYBLEP_OSCILLATOR_HPP
