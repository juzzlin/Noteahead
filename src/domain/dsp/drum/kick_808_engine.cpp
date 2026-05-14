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

//! Shortest and longest -60 dB times the Decay control spans, fitted against uncompressed RD-8 V3
//! recordings. The low end is a short thump, the high end the long sub tail the 808 is wanted for.
constexpr double MinDecayT60 = 0.065;
constexpr double MaxDecayT60 = 2.60;
constexpr double DecayCurve = 0.65;

//! Fixed short excitation pulse time constant. Keeping the body excitation independent of Tone
//! keeps the peak and sub level steady across the entire Tone range (matching the RD-8's <1 dB
//! variation) and avoids the artificial transient spike relative to the body.
constexpr double PulseTau = 0.00055;

//! Pitch envelope time constants, from a snap to an audible downward sweep.
constexpr double MinPitchTau = 0.001;
constexpr double MaxPitchTau = 0.155;

//! Frequency multiplier the pitch envelope reaches at full depth.
constexpr double PitchDepthRange = 8.2;

//! Subtle analog pitch droop over the first ~35 ms as the bridged-T resonator settles.
constexpr double DroopDepth = 0.05;
constexpr double DroopTau = 0.035;

constexpr double MaxGlideSeconds = 0.5;

//! Time constant the residual ring fades over when a new hit takes the voice. Short enough that it
//! cannot beat audibly against the hit that replaced it, long enough to stay a smooth fade.
constexpr double RetriggerFadeSeconds = 0.004;

//! Leaves headroom so a full-velocity hit with drive off does not sit at full scale, while keeping
//! the kit mix level with Drum Synth 808's other pads.
constexpr double OutputGain = 0.81;

//! Peak the drive stage saturates towards.
constexpr double DriveCeiling = 0.5;

//! The click is in two parts, fitted against uncompressed RD-8 V3 recordings:
//! - A zero-mean band-pass body covering 200 Hz - 1 kHz.
//! - A short 2.6 kHz damped sine tick carrying the 1 - 8 kHz snap.
constexpr double ClickBodyFastTau = 0.00084;
constexpr double ClickBodySlowTau = 0.00145;
constexpr double ClickBodyGain = 0.075;

constexpr double ClickTickFrequency = 2600.0;
constexpr double ClickTickT60 = 0.00085;
constexpr double ClickTickGain = 0.62;

//! What Tone leaves of each part at zero: Tone tilts the click and sweeps its low-pass filter
//! rather than switching the click off entirely.
constexpr double ClickBodyFloor = 0.55;
constexpr double ClickTickFloor = 0.16;

//! Tone sweeps the 2-pole Butterworth click low-pass from 1.1 kHz to 9.5 kHz, reproducing the
//! RD-8's ~15 dB high-frequency Tone range while continuing to roll off cleanly towards Nyquist.
constexpr double ClickMinLpfHz = 1100.0;
constexpr double ClickMaxLpfHz = 9500.0;
constexpr double ClickLowPassQ = std::numbers::sqrt2 / 2.0;

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
    m_note = note;
}

void Kick808Engine::setTune(float tune)
{
    m_tune = tune;
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

    m_targetNote = m_note + (m_tune - 0.5f) * 2.0f * TuneRangeSemitones;

    // A silent engine has no pitch to glide from, so the first hit of a phrase starts in tune.
    if (!m_active) {
        m_currentFrequency = targetFrequency();
    }

    m_velocity = velocity;
    m_pulseEnv = 1.0f;
    m_pitchEnv = 1.0f;
    m_droopEnv = 1.0f;
    m_active = true;
    m_stopping = false;

    m_clickSlowEnv = 1.0f;
    m_clickFastEnv = 1.0f;

    // Strike the tick resonator on its real axis. The output tap is the imaginary part, so a hit
    // that lands while the previous tick is still ringing adds to it without stepping the output.
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

    // The pitch envelope and subtle analog droop ride on top of the glided frequency.
    const double droop = DroopDepth * (static_cast<double>(m_pitchDepth) * 2.0) * static_cast<double>(m_droopEnv);
    const double sweptFrequency = m_currentFrequency * (1.0 + static_cast<double>(m_pitchDepth) * PitchDepthRange * static_cast<double>(m_pitchEnv) + droop);
    // Keep the rotation inside the unit circle's stable region even if a caller asks for a silly note.
    const double omega = 2.0 * std::numbers::pi * std::clamp(sweptFrequency, 1.0, sr * 0.45) / sr;
    const double cosOmega = std::cos(omega);
    const double sinOmega = std::sin(omega);

    // Drive the resonator with the excitation pulse, then rotate and damp it.
    const double excitation = static_cast<double>(m_pulseEnv) * static_cast<double>(m_velocity) * m_excitationGain;
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

    const double click = clickBody + m_tickIm;
    const double clickBandLimited = m_clickLowPass.process(click);

    // The click enters against the body's polarity so it reinforces the swept first swing.
    double out = (m_resonatorIm + m_tailIm - clickBandLimited) * OutputGain;

    // Saturation stays fully bypassed at zero drive, so the clean tail is bit-for-bit unaffected.
    if (m_drive > 0.0f) {
        const double gain = 1.0 + static_cast<double>(m_drive) * 24.0;
        const double saturated = std::tanh(out * gain) / std::tanh(gain) * DriveCeiling;
        out += (saturated - out) * static_cast<double>(m_drive);
    }

    m_pulseEnv *= m_pulseDecayRate;
    m_pitchEnv *= m_pitchDecayRate;
    m_droopEnv *= m_droopDecayRate;
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
        m_clickLowPass.reset();
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
    m_droopEnv = 0.0f;
    m_clickSlowEnv = 0.0f;
    m_clickFastEnv = 0.0f;
    m_clickLowPass.reset();
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

    const double t60 = MinDecayT60 * std::pow(MaxDecayT60 / MinDecayT60, std::pow(static_cast<double>(m_decay), DecayCurve));
    m_resonatorDamping = exponentialDecayPerSample(t60 / T60ToTau, sr);
    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds, sr);

    m_pulseTau = PulseTau;
    m_pulseDecayRate = static_cast<float>(exponentialDecayPerSample(m_pulseTau, sr));
    m_excitationGain = 1.0 / (m_pulseTau * sr);

    m_clickSlowDecayRate = static_cast<float>(exponentialDecayPerSample(ClickBodySlowTau, sr));
    m_clickFastDecayRate = static_cast<float>(exponentialDecayPerSample(ClickBodyFastTau, sr));
    m_clickBodyNormalization = 1.0 / (1.0 - ClickBodyFastTau / ClickBodySlowTau);

    const double clickLpfHz = ClickMinLpfHz * std::pow(ClickMaxLpfHz / ClickMinLpfHz, static_cast<double>(m_tone));
    m_clickLowPass.calculateHighCut(clickLpfHz, sr, ClickLowPassQ);

    const double tickOmega = 2.0 * std::numbers::pi * std::clamp(ClickTickFrequency, 1.0, sr * 0.45) / sr;
    m_tickCos = std::cos(tickOmega);
    m_tickSin = std::sin(tickOmega);
    m_tickDamping = exponentialDecayPerSample(ClickTickT60 / T60ToTau, sr);

    const double pitchTau = MinPitchTau * std::pow(MaxPitchTau / MinPitchTau, static_cast<double>(m_pitchDecay));
    m_pitchDecayRate = static_cast<float>(exponentialDecayPerSample(pitchTau, sr));
    m_droopDecayRate = static_cast<float>(exponentialDecayPerSample(DroopTau, sr));

    const double glideSeconds = static_cast<double>(m_glide) * MaxGlideSeconds;
    m_glideRate = glideSeconds > 0.0 ? 1.0 - exponentialDecayPerSample(glideSeconds, sr) : 1.0;
}

} // namespace noteahead
