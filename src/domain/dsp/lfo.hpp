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

#ifndef LFO_HPP
#define LFO_HPP

#include "dsp_component.hpp"

#include <string>
#include <vector>

namespace noteahead {

class Lfo : public DspComponent
{
public:
    enum class Waveform
    {
        Saw = 0,
        Sine = 3,
        Square = 2,
        Triangle = 1,
    };

    static std::vector<std::string> waveformNames();

    enum class Mode
    {
        BPM = 1,
        Normal = 0,
        OneShot = 2,
    };

    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);
    void setWaveform(Waveform waveform);
    void setMode(Mode mode);
    void setPhase(double phase);
    void trigger();
    double nextSample();
    void reset();

private:
    double m_frequency { 1.0 };

    Waveform m_waveform { Waveform::Triangle };
    Mode m_mode { Mode::Normal };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
    bool m_oneShotActive { true };

    void updatePhaseStep();
};

} // namespace noteahead

#endif // LFO_HPP
