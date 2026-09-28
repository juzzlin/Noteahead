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
        //! Falling to silence before a retrigger's attack begins. See trigger().
        Choke,
        Attack,
        Hold,
        Decay,
        //! Held at the sustain level until the note is let go of. Appended, so the names that were
        //! here keep their places.
        Sustain,
        Release
    };

    void setAttackTime(double seconds);
    void setHoldTime(double seconds);
    void setDecayTime(double seconds);

    //! Where the decay lands, 0..1. Zero, the default, decays to silence and goes idle, which is
    //! what a one-shot drum envelope did before the stage existed.
    //!
    //! At one the envelope is flat at the top for as long as the note is held, and with a zero
    //! attack, hold and release it is then a constant 1 -- an envelope that does nothing at all,
    //! which is the only way for V2 to sound exactly like V1 rather than merely close to it.
    void setSustainLevel(double level);
    //! How long the level takes to reach silence once the note is let go of.
    void setReleaseTime(double seconds);

    //! Bend of the attack and the decay, 0..1, with the same meaning and the same shaping as
    //! AdsrEnvelope::setCurve(): zero leaves them straight and anything above moves most of the
    //! travel to the start of the segment.
    void setCurve(double curve);

    void setSampleRate(double sampleRate) override;

    //! Struck. An attack that has somewhere to travel begins at once; one that does not is given
    //! somewhere to travel first.
    //!
    //! A drum voice is one voice rather than a pool, so a hit during the last one's tail has to
    //! reuse the envelope that tail is still riding on. Starting the attack from where the level
    //! stands means a retrigger at full level has no distance to cover, and the attack is silent --
    //! which is why a slow attack was heard on the first hit and never again. Starting it from zero
    //! instead steps the output down to silence, which is a click.
    //!
    //! So the level is walked down to zero over ChokeSeconds first and the attack follows from
    //! there, which is what the Sampler achieves by fading the old voice under a new one. The cost
    //! is that a retrigger is late by that much, and it is set short enough not to be heard as
    //! timing.
    void trigger();
    //! Note off. Falls away from wherever the level stands; a no-op on an envelope already idle.
    void release();
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

    //! How long the level takes to reach zero before a retrigger's attack. Long enough that the
    //! step is a ramp rather than an edge, short enough to sit inside the attack transient of the
    //! drum being struck.
    static constexpr double ChokeSeconds { 0.002 };

    //! Level below which a retrigger simply attacks: there is nothing left to walk down from, and
    //! delaying the hit to fade silence would only make it late.
    static constexpr double ChokeThreshold { 0.001 };

    double m_attackTime { MinimumSegmentTime };
    double m_holdTime { 0.0 };
    double m_sustainLevel { 0.0 };
    double m_releaseTime { 0.0 };
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
