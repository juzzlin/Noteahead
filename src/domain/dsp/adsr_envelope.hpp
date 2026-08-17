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

#include "dsp_component.hpp"

namespace noteahead {

class AdsrEnvelope : public DspComponent
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

    void setAttackTime(double seconds);
    void setDecayTime(double seconds);
    void setSustainLevel(double level);
    void setReleaseTime(double seconds);

    //! Bend of every segment, 0..1. Zero leaves them straight lines, which is what this envelope has
    //! always been, so a patch that never touches this sounds exactly as it did.
    //!
    //! Anything above bends them into the analog shape: most of the travel happens at the start of
    //! the segment and the rest eases in. A pluck needs this. A straight decay is still at -6 dB
    //! halfway through, where a real string is some 20 dB down, and then it falls off a cliff at the
    //! end instead of ringing out — which is why a linear envelope reads as a fade, not an attack.
    void setCurve(double curve);

    void setSampleRate(double sampleRate) override;

    void trigger();
    void release();
    void reset();

    double nextSample();
    double value() const;
    State state() const;
    bool isActive() const;

    //! True once the envelope can no longer produce anything and the note may be dropped.
    //!
    //! Not the same as isActive(): a percussive patch with a zero sustain level decays to silence
    //! and then *parks* in Sustain, which is not Idle. A voice gated only on Idle therefore renders
    //! digital silence forever, at full cost, until the note is released — which in a tracker
    //! pattern may be never.
    bool isSilent() const;

private:
    //! Level below which the envelope is treated as finished, about -120 dBFS.
    static constexpr double SilenceThreshold { 1.0e-6 };

    //! Curvature at the top of the curve knob. Chosen so the middle of its travel already lands on
    //! the natural pluck shape, about 18 dB down at the halfway point of the decay, and the top
    //! third is left for the percussive shapes that need to be gone almost at once.
    static constexpr double MaxCurvature { 8.0 };

    State m_state { State::Idle };

    double m_attackTime { 0.01 };
    double m_decayTime { 0.1 };
    double m_sustainLevel { 1.0 };
    double m_releaseTime { 0.1 };
    double m_curve { 0.0 };

    double m_currentLevel { 0.0 };

    //! Where the running segment started and where it ends. The level is no longer a straight line
    //! between them, so the shaping is evaluated from the segment's own progress instead.
    double m_segmentStart { 0.0 };
    double m_segmentTarget { 0.0 };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };

    void beginSegment(State state);

    //! Seconds the given segment takes from the level the envelope entered it at. Attack and release
    //! both scale with the distance left to travel, which is what the fixed-step envelope did: a
    //! retrigger part way up finishes early rather than stalling at a level it has already reached.
    double segmentDuration(State state) const;

    //! Progress 0..1 mapped through the curve, normalised so the segment still lands exactly on its
    //! target: no asymptotic tail that never arrives, and no early cutoff.
    double shape(double phase) const;

    void updatePhaseStep();
};

} // namespace noteahead

#endif // ADSR_ENVELOPE_HPP
