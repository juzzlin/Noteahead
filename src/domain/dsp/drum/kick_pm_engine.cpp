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

#include "kick_pm_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double T60ToTau = 6.907755;
constexpr double RetriggerFadeSeconds = 0.008;
constexpr double DriveCeiling = 0.7;
constexpr double OutputGain = 0.5;
//! After the drive, which is set against OutputGain: puts a default hit's peak at about -5 dBFS, as
//! the Snare Synth's
constexpr double LevelTrim = 0.54;

//! A Butterworth response from CascadedSvf: k = 2 * (1 - resonance) = sqrt(2)
constexpr double FlatResonance = 0.2929;

//! The batter head's modes as the air inside a kick loads them. Real kicks put their strongest
//! overtones at 1.3-2x the fundamental, not at a free membrane's 1.59, 2.14, 2.30.
constexpr std::array<double, KickPmEngine::BatterModes> BatterRatios { 1.00, 1.47, 1.68, 1.93, 2.24, 2.55, 2.90, 3.30 };
constexpr std::array<double, KickPmEngine::BatterModes> BatterLevels { 1.00, 0.85, 0.75, 0.65, 0.50, 0.40, 0.30, 0.25 };
//! The rise in pitch a full-velocity hit at full Pitch Drop starts with
constexpr double MaxPitchRise = 0.6;
//! The front head is tensioned by the hit less than the batter
constexpr double FrontPitchRiseShare = 0.5;

//! A pillow at full damping puts a Gaussian of this time constant on the heads, as the damped 24"
//! kick the presets are fitted to has
constexpr double MinPillowTau = 0.06;
constexpr double MaxPillowTau = 1.0;

//! The beater's contact time, felt to hard
constexpr double FeltContactSeconds = 0.006;
constexpr double HardContactSeconds = 0.0006;

constexpr double ClickLowHz = 800.0;

double exponentialDecayPerSample(double tau, double sampleRate)
{
    return std::exp(-1.0 / (std::max(tau, 1.0e-6) * sampleRate));
}

double hzToNormalizedCutoff(double hz, double sampleRate)
{
    const double maxFreq = std::min(20000.0, sampleRate * 0.49);
    return std::clamp(std::log2(std::max(20.0, hz) / 20.0) / std::log2(maxFreq / 20.0), 0.0, 1.0);
}

} // namespace

KickPmEngine::KickPmEngine()
{
    m_clickHp.setMode(CascadedSvf::Mode::HighPass);
    m_clickHp.setOrder(2);
    m_clickHp.setResonance(FlatResonance);
    m_clickLp.setMode(CascadedSvf::Mode::LowPass);
    m_clickLp.setOrder(2);
    m_clickLp.setResonance(FlatResonance);

    m_ringHp.setMode(CascadedSvf::Mode::HighPass);
    m_ringHp.setOrder(2);
    m_ringHp.setResonance(FlatResonance);
    m_ringLp.setMode(CascadedSvf::Mode::LowPass);
    m_ringLp.setOrder(2);
    m_ringLp.setResonance(FlatResonance);

    m_shellResonator.setMode(CascadedSvf::Mode::BandPass);
    m_shellResonator.setOrder(2);
    m_shellResonator.setResonance(0.6);

    initModes();
    updateRates();
}

KickPmEngine::~KickPmEngine() = default;

double KickPmEngine::contactSeconds() const
{
    const double beater = static_cast<double>(std::clamp(m_beater, 0.0f, 1.0f));
    return FeltContactSeconds * std::pow(HardContactSeconds / FeltContactSeconds, beater);
}

double KickPmEngine::contactWeight(double hz) const
{
    // The spectrum of a half-sine pulse of length T, against its value at DC:
    // cos(pi f T) / (1 - (2 f T)^2), which is pi / 4 where the denominator vanishes
    const double x = 2.0 * hz * contactSeconds();
    const double denominator = 1.0 - x * x;
    if (std::abs(denominator) < 1.0e-3) {
        return std::numbers::pi / 4.0;
    }
    return std::abs(std::cos(std::numbers::pi * hz * contactSeconds()) / denominator);
}

void KickPmEngine::initModes()
{
    const double pos = static_cast<double>(std::clamp(m_position, 0.0f, 1.0f));
    const double front = static_cast<double>(std::clamp(m_frontHead, 0.0f, 1.0f));
    const double outside = static_cast<double>(std::clamp(m_mic, 0.0f, 1.0f));
    const double tune = static_cast<double>(m_tune);

    // Off centre the higher modes are reached, at the centre mostly the fundamental
    for (int i = 0; i < BatterModes; i++) {
        m_modes[i].ratio = BatterRatios[i];
        const double placement = i == 0 ? 1.0 - 0.3 * pos : 0.3 + 0.9 * pos;
        // Inside, the mic is close to the batter head and hears its overtones
        const double micWeight = i == 0 ? 1.0 : 1.0 - 0.4 * outside;
        m_modes[i].amplitude = BatterLevels[i] * placement * micWeight * contactWeight(tune * BatterRatios[i]);
    }
    // The front head is driven through the air, so it carries the low modes. A port lets the air
    // out and it hardly moves; outside, the mic is right in front of it.
    const double frontLevel = (0.1 + 0.9 * front) * (0.4 + 0.6 * outside);
    for (int i = 0; i < FrontModes; i++) {
        auto & mode = m_modes[BatterModes + i];
        mode.ratio = BatterRatios[i] * static_cast<double>(m_frontTune);
        mode.amplitude = 0.6 * BatterLevels[i] * frontLevel * contactWeight(tune * BatterRatios[i]);
    }
    m_referenceAmplitude = std::max(1.0e-6, m_modes[0].amplitude);
}

void KickPmEngine::updateRates()
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }
    m_lastSampleRate = sr;

    initModes();

    const double tau0 = std::max(0.02, static_cast<double>(m_decay)) / T60ToTau;
    const double toneSlope = 0.5 + (1.0 - static_cast<double>(m_tone)) * 5.0;
    const double front = static_cast<double>(std::clamp(m_frontHead, 0.0f, 1.0f));
    for (int i = 0; i < TotalModes; i++) {
        const bool isFront = i >= BatterModes;
        const double ratio = isFront ? BatterRatios[i - BatterModes] : BatterRatios[i];
        double tau = tau0 / (1.0 + (ratio - 1.0) * toneSlope);
        if (isFront) {
            // A ported head lets the air go and dies quickly; a solid one is the boom that outlasts
            // the batter
            tau *= 0.2 + 1.8 * front;
        }
        m_modes[i].damping = exponentialDecayPerSample(tau, sr);
    }

    // exp(-(n / (tau * sr))^2): each sample's ratio to the last is exp(-(2n + 1) * a)
    const double damping = static_cast<double>(std::clamp(m_damping, 0.0f, 1.0f));
    if (damping > 0.0) {
        const double pillowTau = MinPillowTau + (MaxPillowTau - MinPillowTau) * (1.0 - damping) * (1.0 - damping);
        const double a = 1.0 / std::pow(pillowTau * sr, 2.0);
        m_pillowFirstRatio = std::exp(-a);
        m_pillowRatioStep = std::exp(-2.0 * a);
    } else {
        m_pillowFirstRatio = 1.0;
        m_pillowRatioStep = 1.0;
    }

    // A hard beater clicks higher and shorter than felt
    const double beater = static_cast<double>(std::clamp(m_beater, 0.0f, 1.0f));
    const double clickHighHz = 1500.0 + 8000.0 * beater;
    const double clickTau = 0.012 - 0.009 * beater;
    m_clickDecayRate = exponentialDecayPerSample(clickTau, sr);
    m_clickHp.setSampleRate(sr);
    m_clickHp.setCutoff(hzToNormalizedCutoff(ClickLowHz, sr));
    m_clickLp.setSampleRate(sr);
    m_clickLp.setCutoff(hzToNormalizedCutoff(clickHighHz, sr));

    // The shell's knock sits a few times above the head's pitch, where the wood resonates
    const double shellHz = std::clamp(static_cast<double>(m_tune) * 4.5, 120.0, 600.0);
    m_shellResonator.setSampleRate(sr);
    m_shellResonator.setCutoff(hzToNormalizedCutoff(shellHz, sr));

    // The upper modes start a few times above the fundamental and fall away with Tone
    m_ringDecayRate = exponentialDecayPerSample(std::max(0.01, static_cast<double>(m_ringDecay)) / T60ToTau, sr);
    m_ringHp.setSampleRate(sr);
    m_ringHp.setCutoff(hzToNormalizedCutoff(std::max(150.0, static_cast<double>(m_tune) * 4.0), sr));
    m_ringLp.setSampleRate(sr);
    m_ringLp.setCutoff(hzToNormalizedCutoff(1000.0 + 7000.0 * static_cast<double>(m_tone), sr));

    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds / T60ToTau, sr);
    m_noiseBank.setOversampleFactor(oversampleFactor());
}

void KickPmEngine::trigger(float velocity)
{
    if (sampleRate() != m_lastSampleRate) {
        updateRates();
    }
    m_velocity = std::clamp(velocity, 0.0f, 1.0f);
    m_active = true;
    // Before the pillow is lifted: the tail takes the drum as it sounds now
    triggerModes(m_velocity);
    m_pillowEnv = 1.0;
    m_pillowRatio = m_pillowFirstRatio;
    // The click grows faster than the hit: a soft kick is nearly all thump
    m_clickEnv = static_cast<double>(m_velocity) * static_cast<double>(m_velocity);
    m_ringEnv = static_cast<double>(m_velocity);
}

void KickPmEngine::triggerModes(float velocity)
{
    const double vel = static_cast<double>(velocity);
    for (auto && mode : m_modes) {
        // The pillow has pulled the old hit down; without it the tail would jump back up
        mode.tailRe += mode.re * m_pillowEnv;
        mode.tailIm += mode.im * m_pillowEnv;
        mode.re = vel * mode.amplitude;
        mode.im = 0.0;
    }
}

float KickPmEngine::nextSample()
{
    float left = 0.0f;
    float right = 0.0f;
    nextSample(left, right);
    return (left + right) * 0.5f;
}

void KickPmEngine::nextSample(float & left, float & right)
{
    if (!m_active) {
        left = 0.0f;
        right = 0.0f;
        return;
    }

    const double sr = sampleRate();
    if (sr != m_lastSampleRate) {
        updateRates();
    }

    // The head is as much tighter as it is displaced
    const double displacement = std::hypot(m_modes[0].re, m_modes[0].im) * m_pillowEnv / m_referenceAmplitude;
    const double rise = MaxPitchRise * static_cast<double>(m_pitchDrop) * std::min(displacement, 1.5);
    const double tune = static_cast<double>(m_tune);

    double batterHead = 0.0;
    double frontHead = 0.0;
    double totalEnergy = 0.0;
    for (int i = 0; i < TotalModes; i++) {
        auto & mode = m_modes[i];
        const bool isFront = i >= BatterModes;
        const double freq = std::clamp(tune * mode.ratio * (1.0 + rise * (isFront ? FrontPitchRiseShare : 1.0)), 20.0, sr * 0.48);
        const double omega = 2.0 * std::numbers::pi * freq / sr;
        const double cosOmega = std::cos(omega);
        const double sinOmega = std::sin(omega);

        const double re = mode.re;
        const double im = mode.im;
        mode.re = mode.damping * (re * cosOmega - im * sinOmega);
        mode.im = mode.damping * (re * sinOmega + im * cosOmega);

        const double tre = mode.tailRe;
        const double tim = mode.tailIm;
        if (std::abs(tre) > 1.0e-7 || std::abs(tim) > 1.0e-7) {
            mode.tailRe = m_tailDamping * (tre * cosOmega - tim * sinOmega);
            mode.tailIm = m_tailDamping * (tre * sinOmega + tim * cosOmega);
        } else {
            mode.tailRe = 0.0;
            mode.tailIm = 0.0;
        }

        // The tail of a retriggered hit is not under the new hit's pillow
        const double out = mode.im * m_pillowEnv + mode.tailIm;
        (isFront ? frontHead : batterHead) += out;
        totalEnergy += (std::abs(mode.re) + std::abs(mode.im)) * m_pillowEnv + std::abs(mode.tailRe) + std::abs(mode.tailIm);
    }
    m_pillowEnv *= m_pillowRatio;
    m_pillowRatio *= m_pillowRatioStep;

    double click = 0.0;
    if (m_click > 0.0f && m_clickEnv > 1.0e-6) {
        if (m_noiseBank.needsBaseSample()) {
            m_noiseBank.setBaseSample(m_dist(m_rng));
        }
        const double noise = static_cast<double>(m_noiseBank.nextSample());
        // Inside, the mic is next to the beater
        const double micWeight = 1.0 - 0.65 * static_cast<double>(m_mic);
        click = m_clickLp.process(m_clickHp.process(noise)) * m_clickEnv * static_cast<double>(m_click) * micWeight;
        m_clickEnv *= m_clickDecayRate;
    }

    double ring = 0.0;
    if (m_ring > 0.0f && m_ringEnv > 1.0e-6) {
        if (m_noiseBank.needsBaseSample()) {
            m_noiseBank.setBaseSample(m_dist(m_rng));
        }
        const double noise = static_cast<double>(m_noiseBank.nextSample());
        ring = m_ringLp.process(m_ringHp.process(noise)) * m_ringEnv * m_pillowEnv * static_cast<double>(m_ring);
        m_ringEnv *= m_ringDecayRate;
    }

    const double shell = m_shellResonator.process(batterHead + click) * static_cast<double>(m_shell) * 2.0;
    double out = (batterHead + frontHead + click + ring + shell) * OutputGain;

    if (m_drive > 0.0f) {
        const double gain = 1.0 + static_cast<double>(m_drive) * 8.0;
        const double saturated = std::tanh(out * gain) / std::tanh(gain) * DriveCeiling;
        out += (saturated - out) * static_cast<double>(m_drive);
    }

    left = static_cast<float>(out * LevelTrim);
    right = left;

    totalEnergy += (m_click > 0.0f ? m_clickEnv : 0.0) + (m_ring > 0.0f ? m_ringEnv * m_pillowEnv : 0.0);
    if (totalEnergy < AmplitudeThreshold) {
        m_active = false;
        reset();
    }
}

bool KickPmEngine::isActive() const
{
    return m_active;
}

void KickPmEngine::reset()
{
    for (auto && mode : m_modes) {
        mode.re = 0.0;
        mode.im = 0.0;
        mode.tailRe = 0.0;
        mode.tailIm = 0.0;
    }
    m_pillowEnv = 1.0;
    m_clickEnv = 0.0;
    m_ringEnv = 0.0;
    m_ringHp.reset();
    m_ringLp.reset();
    m_clickHp.reset();
    m_clickLp.reset();
    m_shellResonator.reset();
    m_noiseBank.reset();
    m_rng.seed(RngSeed);
}

void KickPmEngine::stop()
{
    m_active = false;
    reset();
}

void KickPmEngine::setTune(float tuneHz)
{
    m_tune = tuneHz;
    updateRates();
}

void KickPmEngine::setDecay(float decaySeconds)
{
    m_decay = decaySeconds;
    updateRates();
}

void KickPmEngine::setTone(float tone)
{
    m_tone = tone;
    updateRates();
}

void KickPmEngine::setPitchDrop(float drop)
{
    m_pitchDrop = drop;
}

void KickPmEngine::setDamping(float damping)
{
    m_damping = damping;
    updateRates();
}

void KickPmEngine::setBeater(float beater)
{
    m_beater = beater;
    updateRates();
}

void KickPmEngine::setClick(float click)
{
    m_click = click;
}

void KickPmEngine::setPosition(float position)
{
    m_position = position;
    initModes();
}

void KickPmEngine::setFrontHead(float frontHead)
{
    m_frontHead = frontHead;
    updateRates();
}

void KickPmEngine::setFrontTune(float ratio)
{
    m_frontTune = ratio;
    initModes();
}

void KickPmEngine::setShell(float shell)
{
    m_shell = shell;
}

void KickPmEngine::setMic(float mic)
{
    m_mic = mic;
    initModes();
}

void KickPmEngine::setRing(float ring)
{
    m_ring = ring;
}

void KickPmEngine::setRingDecay(float seconds)
{
    m_ringDecay = seconds;
    updateRates();
}

void KickPmEngine::setDrive(float drive)
{
    m_drive = drive;
}

} // namespace noteahead
