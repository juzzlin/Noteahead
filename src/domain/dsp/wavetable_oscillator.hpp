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

#ifndef WAVETABLE_OSCILLATOR_HPP
#define WAVETABLE_OSCILLATOR_HPP

#include "dsp_component.hpp"
#include "wavetable.hpp"

#include <memory>

namespace noteahead {

class WavetableOscillator : public DspComponent
{
public:
    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);
    void setPosition(double position); // 0.0 to 1.0
    void setWavetable(Wavetable::WavetableCS wavetable);

    double nextSample();
    void sync(double phase);

    double frequency() const;
    double position() const;
    double phase() const;

private:
    Wavetable::WavetableCS m_wavetable;
    double m_frequency = 440.0;
    double m_position = 0.0;
    double m_phase = 0.0;
    double m_phaseStep = 0.0;

    void updatePhaseStep();
};

} // namespace noteahead

#endif // WAVETABLE_OSCILLATOR_HPP
