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

#ifndef ADSR_ENVELOPE_HPP
#define ADSR_ENVELOPE_HPP

#include <cstdint>

namespace noteahead {

class ADSREnvelope
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    void setSampleRate(double sampleRate);
    void setAttackTime(double seconds);
    void setDecayTime(double seconds);
    void setSustainLevel(double level);
    void setReleaseTime(double seconds);

    void trigger();
    void release();
    void reset();

    double nextSample();
    double value() const;
    State state() const;
    bool isActive() const;

private:
    State m_state { State::Idle };
    double m_sampleRate { 44100.0 };

    double m_attackTime { 0.01 };
    double m_decayTime { 0.1 };
    double m_sustainLevel { 1.0 };
    double m_releaseTime { 0.1 };

    double m_currentLevel { 0.0 };
    double m_attackStep { 0.0 };
    double m_decayStep { 0.0 };
    double m_releaseStep { 0.0 };

    void calculateSteps();
};

} // namespace noteahead

#endif // ADSR_ENVELOPE_HPP
