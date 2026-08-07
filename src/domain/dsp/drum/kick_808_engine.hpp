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

#ifndef KICK_808_ENGINE_HPP
#define KICK_808_ENGINE_HPP

#include "../svf_filter.hpp"
#include "drum_engine.hpp"

namespace noteahead {

//! TR-808-style bass drum: a high-Q resonator rung by a short pulse, rather than the pitch-swept
//! oscillator multiplied by an amplitude envelope that KickEngine (the 909 lineage) uses.
//!
//! The bridged-T network in the hardware is a damped resonator that the trigger pulse excites, and
//! modelling it that way rather than as osc * env buys three things that matter for this device:
//!
//! - Decay is the resonator's own damping, so the long tails an 808 is wanted for come out of the
//!   structure instead of a separately tuned envelope.
//! - The excitation pulse shapes the attack, which is what the Tone control acts on.
//! - Retriggering does not have to reset a phase, so it needs none of the discontinuity patching
//!   that KickEngine does with m_retriggerOffset.
//!
//! Being monophonic, a new hit takes the voice over rather than piling onto it. The ring that is
//! still sounding is handed to a separate tail resonator that fades out over a few milliseconds
//! while the main resonator restarts from zero. Letting the old ring stay and simply summing the
//! new pulse into it sounds wrong: the phase the residual happens to be at when the next hit lands
//! depends on the interval between hits, so hits come out alternately reinforced and hollowed in a
//! repeating pattern. Restarting from zero makes every hit identical, and handing the residual to
//! the tail rather than zeroing it keeps the output continuous so nothing clicks.
//!
//! What the resonator alone cannot give is the attack. Third-octave measurements of an RD-8 put its
//! hit tens of decibels above a bare resonator from 200 Hz up, so the click is generated separately:
//! a zero-mean band pass burst covering 200 Hz - 1 kHz plus a short damped sine for the top end,
//! both tilted by Tone and both entering against the body's polarity, where they reinforce the
//! swept first swing rather than cancelling it.
//!
//! The resonator is in coupled (rotating phasor) form: each sample rotates the state vector by the
//! current angular frequency and scales it by the damping factor. Unlike a direct-form biquad the
//! coupled form stays stable and click-free when the frequency is modulated per sample, which both
//! the pitch envelope and the glide do continuously.
class Kick808Engine : public DrumEngine
{
public:
    Kick808Engine();
    ~Kick808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    //! Target pitch of the next trigger, in MIDI note units. Fractional values are allowed so the
    //! device can fold its tuning offset in without quantizing to semitones.
    void setNote(float note);

    void setDecay(float decay);
    void setTone(float tone);
    void setPitchDepth(float depth);
    void setPitchDecay(float decay);
    void setDrive(float drive);
    void setGlide(float glide);

private:
    void updateRates();
    double targetFrequency() const;
    //! Tone mapped onto [floorFraction, 1], so a control can be tilted by Tone without being
    //! switched off at the bottom of its range.
    double toneScaled(double floorFraction) const;
    static double noteToFrequency(double note);

    // Resonator state as a rotating phasor. m_resonatorIm is the output tap.
    double m_resonatorRe { 0.0 };
    double m_resonatorIm { 0.0 };

    // Whatever the previous hit was still ringing, fading out from under the current one. It is
    // rotated at the current frequency rather than the one it was struck at, which is inaudible
    // over a fade this short.
    double m_tailRe { 0.0 };
    double m_tailIm { 0.0 };

    // Two poles holding the click's step down to a rolloff the hardware also has.
    SvfFilter m_clickLowPass;

    // The click's tick, a third phasor at a fixed frequency. m_tickIm is its output tap.
    double m_tickRe { 0.0 };
    double m_tickIm { 0.0 };

    // Frequency actually rendered, in Hz. Chases m_targetNote when glide is engaged.
    double m_currentFrequency { 0.0 };
    float m_targetNote { 36.0f };

    float m_pulseEnv { 0.0f };
    float m_pitchEnv { 0.0f };
    float m_clickSlowEnv { 0.0f };
    float m_clickFastEnv { 0.0f };
    float m_velocity { 1.0f };
    bool m_active { false };
    bool m_stopping { false };

    float m_decay { 0.6f };
    float m_tone { 0.35f };
    float m_pitchDepth { 0.5f };
    float m_pitchDecay { 0.25f };
    float m_drive { 0.0f };
    float m_glide { 0.0f };

    // Per-sample multipliers, recomputed whenever a rate-defining parameter or the sample rate moves.
    double m_resonatorDamping { 0.0 };
    double m_tailDamping { 0.0 };
    double m_tickDamping { 0.0 };
    double m_tickCos { 0.0 };
    double m_tickSin { 0.0 };
    float m_pulseDecayRate { 0.0f };
    float m_clickSlowDecayRate { 0.0f };
    float m_clickFastDecayRate { 0.0f };
    double m_clickBodyNormalization { 1.0 };
    float m_pitchDecayRate { 0.0f };
    double m_excitationGain { 0.0 };
    double m_excitationCompensation { 1.0 };
    double m_pulseTau { 0.0 };
    double m_glideRate { 0.0 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // KICK_808_ENGINE_HPP
