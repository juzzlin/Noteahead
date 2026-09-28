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

#ifndef DRUM_AMP_ENVELOPE_HPP
#define DRUM_AMP_ENVELOPE_HPP

#include "../dsp_component.hpp"

namespace noteahead {

//! One-shot amplitude envelope for a drum voice: attack up, hold at the top, decay away, done.
//!
//! Attack/Hold/Decay rather than the ADSR the pitched devices use, because a drum is struck rather
//! than held: there is no gate to sustain under and a note-off only ever chokes. What the engine
//! plays is shaped by this rather than replaced by it -- the engine still has its own decay, and
//! this rides on top, which is what makes it a way to tighten a drum rather than a second voice.
class DrumAmpEnvelope : public DspComponent
{
public:
    enum class State
    {
        Idle,
        Attack,
        Hold,
        Decay
    };

    void setAttackTime(double seconds);
    void setHoldTime(double seconds);
    void setDecayTime(double seconds);

    //! Bend of the attack and the decay, 0..1, with the same meaning and the same shaping as
    //! AdsrEnvelope::setCurve(): zero leaves them straight and anything above moves most of the
    //! travel to the start of the segment.
    void setCurve(double curve);

    void setSampleRate(double sampleRate) override;

    void trigger();
    void reset();

    double nextSample();
    double value() const;
    State state() const;

    //! False once the decay has run out, which is what tells the voice to stop rendering. An
    //! envelope that has never been triggered is not active either.
    bool isActive() const;

private:
    double segmentDuration(State state) const;
    void beginSegment(State state);
    void updatePhaseStep();
    double shape(double phase) const;

    static constexpr double MaxCurvature { 6.0 };
    static constexpr double MinimumSegmentTime { 0.000001 };

    double m_attackTime { MinimumSegmentTime };
    double m_holdTime { 0.0 };
    double m_decayTime { MinimumSegmentTime };
    double m_curve { 0.0 };

    State m_state { State::Idle };
    double m_currentLevel { 0.0 };
    double m_segmentStart { 0.0 };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
};

} // namespace noteahead

#endif // DRUM_AMP_ENVELOPE_HPP
