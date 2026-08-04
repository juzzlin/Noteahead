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

#include "kick_808_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Shortest and longest -60 dB times the Decay control spans. The low end is a bare click, the high
//! end the long sub tail the 808 is wanted for.
constexpr double MinDecayT60 = 0.04;
constexpr double MaxDecayT60 = 3.0;

//! Excitation pulse time constants. A shorter pulse is a broader spectrum, so Tone brightens the
//! attack by shortening it.
constexpr double MaxPulseTau = 0.004;
constexpr double MinPulseTau = 0.0005;

//! Pitch envelope time constants, from a snap to an audible downward sweep.
constexpr double MinPitchTau = 0.005;
constexpr double MaxPitchTau = 0.155;

//! Frequency multiplier the pitch envelope reaches at full depth.
constexpr double PitchDepthRange = 3.0;

constexpr double MaxGlideSeconds = 0.5;

//! Time constant the residual ring fades over when a new hit takes the voice. Short enough that it
//! cannot beat audibly against the hit that replaced it, long enough to stay a smooth fade.
constexpr double RetriggerFadeSeconds = 0.004;

//! Leaves headroom so a full-velocity hit with drive off does not sit at full scale. Re-based when
//! the excitation gained its pulse-length compensation, which lifted the quiet end of Tone's range
//! by some 10 dB rather than lowering the loud end.
constexpr double OutputGain = 0.5;

//! Peak the drive stage saturates towards.
//!
//! The saturator normalises by tanh(gain), which maps anything with level asymptotically to full
//! scale, so without a ceiling under 1.0 a fully driven hit pins at exactly +/-1.0. That is bad in
//! two ways: it sits on the device rack's clip indicator permanently, since the indicator latches
//! at full scale and the next buffer re-latches it as fast as it can be cleared, and it leaves
//! nothing for the fader or an off-centre pan, both of which can only push it further over.
//!
//! Aiming here instead also keeps Drive from doubling as a +10 dB makeup control, so the knob
//! changes the harmonics rather than the level.
constexpr double DriveCeiling = 0.5;

//! The click is in two parts, which is what an RD-8 measures like.
//!
//! Third-octave band energies of a hit, taken against that machine, put its attack 5 - 18 dB above
//! ours through 200 Hz - 800 Hz and 7 - 27 dB above it everywhere over 200 Hz once its Tone is
//! backed off. Above 800 Hz with Tone up we were already close. So the gap is a missing mid-band
//! transient, not a missing top end, and the hardware clicks with Tone at zero where we did not
//! click at all.
//!
//! The body of the click carries 200 Hz - 1 kHz. Weighting the slow term by the ratio of the two
//! time constants makes the shape integrate to zero, so the click is a band pass burst with nothing
//! left under the fundamental: a one-sided blip carries DC and sub instead, which turned Tone into
//! a 10 dB level control down there.
//!
//! Its length used to be the excitation pulse's, which Tone shortens. That pushed its corner up to
//! about 1 kHz at full Tone, leaving a hole between the fundamental and the top end, and it made
//! the click carry *less* energy the further Tone was opened.
constexpr double ClickBodyFastTau = 0.00084;
constexpr double ClickBodySlowTau = 0.00145;
constexpr double ClickBodyGain = 0.27;

//! The tick is a damped sine burst about two cycles long, which is the top end of the measurement.
//! Being bipolar it leaves nothing at DC for the device's blocker to throw away.
constexpr double ClickTickFrequency = 830.0;
constexpr double ClickTickT60 = 0.0024;
constexpr double ClickTickGain = 0.40;

//! What Tone leaves of each part at zero. The hardware clicks across its whole range - Tone is a
//! tilt on the click rather than an on/off switch for it - so neither part backs out entirely.
constexpr double ClickBodyFloor = 0.30;
constexpr double ClickTickFloor = 0.29;

//! Corner the click is band limited at. Chosen against the RD-8's own top octaves rather than as
//! high as possible: the click is a step, and a step has to be rolled off somewhere.
constexpr double ClickLowPassFrequency = 8000.0;

//! Ratio between a -60 dB time and the exponential time constant that produces it.
constexpr double T60ToTau = 6.907755;

double exponentialDecayPerSample(double tau, double sampleRate)
{
    return std::exp(-1.0 / (std::max(tau, 1.0e-6) * sampleRate));
}

} // namespace

Kick808Engine::Kick808Engine()
{
    updateRates();
}

void Kick808Engine::setNote(float note)
{
    m_targetNote = note;
}

void Kick808Engine::setDecay(float decay)
{
    m_decay = decay;
    updateRates();
}

void Kick808Engine::setTone(float tone)
{
    m_tone = tone;
    updateRates();
}

void Kick808Engine::setPitchDepth(float depth)
{
    m_pitchDepth = depth;
}

void Kick808Engine::setPitchDecay(float decay)
{
    m_pitchDecay = decay;
    updateRates();
}

void Kick808Engine::setDrive(float drive)
{
    m_drive = drive;
}

void Kick808Engine::setGlide(float glide)
{
    m_glide = glide;
    updateRates();
}

double Kick808Engine::toneScaled(double floorFraction) const
{
    return floorFraction + (1.0 - floorFraction) * static_cast<double>(m_tone);
}

double Kick808Engine::noteToFrequency(double note)
{
    return 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
}

double Kick808Engine::targetFrequency() const
{
    return noteToFrequency(static_cast<double>(m_targetNote));
}

void Kick808Engine::trigger(float velocity)
{
    if (const double sr = sampleRate(); sr != m_lastSampleRate) {
        updateRates();
    }

    // A silent engine has no pitch to glide from, so the first hit of a phrase starts in tune.
    if (!m_active) {
        m_currentFrequency = targetFrequency();
    }

    m_velocity = velocity;
    m_pulseEnv = 1.0f;
    m_pitchEnv = 1.0f;
    m_active = true;
    m_stopping = false;

    m_clickSlowEnv = 1.0f;
    m_clickFastEnv = 1.0f;

    // The pulse rings the resonator through what amounts to a one-pole low pass at the frequency
    // the hit starts on, which is the swept one rather than the note's own.
    const double sweptStart = m_currentFrequency * (1.0 + static_cast<double>(m_pitchDepth) * PitchDepthRange);
    m_excitationCompensation = std::hypot(1.0, 2.0 * std::numbers::pi * sweptStart * m_pulseTau);

    // Strike the tick resonator on its real axis. The output tap is the imaginary part, so a hit
    // that lands while the previous tick is still ringing adds to it without stepping the output,
    // and no separate fade is needed for a burst this short.
    m_tickRe += toneScaled(ClickTickFloor) * static_cast<double>(velocity) * ClickTickGain;

    // Hand the ring that is still sounding to the tail and restart the resonator from zero, so every
    // hit is identical regardless of what the previous one was doing. Accumulating rather than
    // overwriting keeps the output continuous across hits that arrive faster than the fade.
    m_tailRe += m_resonatorRe;
    m_tailIm += m_resonatorIm;
    m_resonatorRe = 0.0;
    m_resonatorIm = 0.0;
}

float Kick808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr = sampleRate();
    if (sr != m_lastSampleRate) {
        updateRates();
    }

    // Glide the rendered frequency toward the note that was last triggered.
    m_currentFrequency += (targetFrequency() - m_currentFrequency) * m_glideRate;

    // The pitch envelope rides on top of the glided frequency.
    const double sweptFrequency = m_currentFrequency * (1.0 + static_cast<double>(m_pitchDepth) * PitchDepthRange * static_cast<double>(m_pitchEnv));
    // Keep the rotation inside the unit circle's stable region even if a caller asks for a silly note.
    const double omega = 2.0 * std::numbers::pi * std::clamp(sweptFrequency, 1.0, sr * 0.45) / sr;
    const double cosOmega = std::cos(omega);
    const double sinOmega = std::sin(omega);

    // Drive the resonator with the excitation pulse, then rotate and damp it.
    const double excitation = static_cast<double>(m_pulseEnv) * static_cast<double>(m_velocity) * m_excitationGain * m_excitationCompensation;
    const double re = m_resonatorRe + excitation;
    const double im = m_resonatorIm;
    const double chokeDamping = m_stopping ? exponentialDecayPerSample(ChokeFadeSeconds, sr) : 1.0;
    const double damping = m_resonatorDamping * chokeDamping;
    m_resonatorRe = damping * (re * cosOmega - im * sinOmega);
    m_resonatorIm = damping * (re * sinOmega + im * cosOmega);

    // The previous hit's residual rotates alongside it, on its own much faster fade.
    const double tailDamping = m_tailDamping * chokeDamping;
    const double tailRe = m_tailRe;
    m_tailRe = tailDamping * (tailRe * cosOmega - m_tailIm * sinOmega);
    m_tailIm = tailDamping * (tailRe * sinOmega + m_tailIm * cosOmega);

    // The click is the excitation heard directly rather than part of the drum's body, so neither of
    // its parts is swept by the pitch envelope or glided.
    const double clickBody = static_cast<double>(m_clickFastEnv - ClickBodyFastTau / ClickBodySlowTau * m_clickSlowEnv) * m_clickBodyNormalization * static_cast<double>(m_velocity) * toneScaled(ClickBodyFloor) * ClickBodyGain;

    const double tickRe = m_tickRe;
    const double tickDamping = m_tickDamping * chokeDamping;
    m_tickRe = tickDamping * (tickRe * m_tickCos - m_tickIm * m_tickSin);
    m_tickIm = tickDamping * (tickRe * m_tickSin + m_tickIm * m_tickCos);

    // Both parts of the click start from zero in a single sample, and a step like that carries level
    // all the way to Nyquist: without this the top two octaves come out flat instead of continuing
    // to fall, which reads as a small shelf climbing towards 20 kHz on an analyser. One pole is
    // enough to put the rolloff back where the hardware's is, and it is far enough above the click
    // itself to leave the rest of the spectrum alone.
    const double click = clickBody + m_tickIm;
    m_clickLowPass += (click - m_clickLowPass) * m_clickLowPassRate;

    // The click enters against the body's polarity. The resonator's first swing under the pitch
    // envelope is what puts energy around 200 Hz, and a click of the same sign partly cancels it -
    // measured as a 10 dB hole right where the hardware is strongest. Inverted, the two reinforce.
    double out = (m_resonatorIm + m_tailIm - m_clickLowPass) * OutputGain;

    // Saturation stays fully bypassed at zero drive, so the clean tail is bit-for-bit unaffected.
    if (m_drive > 0.0f) {
        const double gain = 1.0 + static_cast<double>(m_drive) * 24.0;
        const double saturated = std::tanh(out * gain) / std::tanh(gain) * DriveCeiling;
        out += (saturated - out) * static_cast<double>(m_drive);
    }

    m_pulseEnv *= m_pulseDecayRate;
    m_pitchEnv *= m_pitchDecayRate;
    m_clickSlowEnv *= m_clickSlowDecayRate;
    m_clickFastEnv *= m_clickFastDecayRate;

    const double amplitude = std::hypot(m_resonatorRe, m_resonatorIm) + std::hypot(m_tailRe, m_tailIm) + std::hypot(m_tickRe, m_tickIm);
    if (amplitude < AmplitudeThreshold && m_pulseEnv < AmplitudeThreshold && m_clickSlowEnv < AmplitudeThreshold) {
        m_active = false;
        m_resonatorRe = 0.0;
        m_resonatorIm = 0.0;
        m_tailRe = 0.0;
        m_tailIm = 0.0;
        m_tickRe = 0.0;
        m_tickIm = 0.0;
        m_pulseEnv = 0.0f;
        m_clickSlowEnv = 0.0f;
        m_clickFastEnv = 0.0f;
        m_clickLowPass = 0.0;
    }

    return static_cast<float>(out);
}

bool Kick808Engine::isActive() const
{
    return m_active;
}

void Kick808Engine::reset()
{
    m_active = false;
    m_stopping = false;
    m_resonatorRe = 0.0;
    m_resonatorIm = 0.0;
    m_tailRe = 0.0;
    m_tailIm = 0.0;
    m_tickRe = 0.0;
    m_tickIm = 0.0;
    m_pulseEnv = 0.0f;
    m_pitchEnv = 0.0f;
    m_clickSlowEnv = 0.0f;
    m_clickFastEnv = 0.0f;
    m_clickLowPass = 0.0;
    m_currentFrequency = 0.0;
}

void Kick808Engine::stop()
{
    m_stopping = true;
}

void Kick808Engine::updateRates()
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }

    m_lastSampleRate = sr;

    const double t60 = MinDecayT60 * std::pow(MaxDecayT60 / MinDecayT60, static_cast<double>(m_decay));
    m_resonatorDamping = exponentialDecayPerSample(t60 / T60ToTau, sr);
    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds, sr);

    m_pulseTau = MaxPulseTau + (MinPulseTau - MaxPulseTau) * static_cast<double>(m_tone);
    m_pulseDecayRate = static_cast<float>(exponentialDecayPerSample(m_pulseTau, sr));

    // Normalize the injected energy against the pulse length so Tone changes the attack's colour
    // rather than the level of the tail behind it. Matching the area alone does not achieve that:
    // the injection is spread over the pulse, so once it is no longer short against the resonator's
    // own period it starts averaging against itself. That left Tone worth 9 dB of level at the
    // fundamental, measured, which is the resonator's one-pole response to the pulse. trigger()
    // divides that response back out, at the frequency the hit actually starts on.
    m_excitationGain = 1.0 / (m_pulseTau * sr);

    m_clickSlowDecayRate = static_cast<float>(exponentialDecayPerSample(ClickBodySlowTau, sr));
    m_clickFastDecayRate = static_cast<float>(exponentialDecayPerSample(ClickBodyFastTau, sr));

    // Normalize the shape by its own peak, which is at t = 0, so ClickBodyGain stays the click's
    // level rather than something the time constants quietly scale.
    m_clickBodyNormalization = 1.0 / (1.0 - ClickBodyFastTau / ClickBodySlowTau);

    m_clickLowPassRate = 1.0 - std::exp(-2.0 * std::numbers::pi * std::min(ClickLowPassFrequency, sr * 0.45) / sr);

    const double tickOmega = 2.0 * std::numbers::pi * std::clamp(ClickTickFrequency, 1.0, sr * 0.45) / sr;
    m_tickCos = std::cos(tickOmega);
    m_tickSin = std::sin(tickOmega);
    m_tickDamping = exponentialDecayPerSample(ClickTickT60 / T60ToTau, sr);

    const double pitchTau = MinPitchTau + (MaxPitchTau - MinPitchTau) * static_cast<double>(m_pitchDecay);
    m_pitchDecayRate = static_cast<float>(exponentialDecayPerSample(pitchTau, sr));

    const double glideSeconds = static_cast<double>(m_glide) * MaxGlideSeconds;
    m_glideRate = glideSeconds > 0.0 ? 1.0 - exponentialDecayPerSample(glideSeconds, sr) : 1.0;
}

} // namespace noteahead
