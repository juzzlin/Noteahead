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

#include "snare_pm_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double T60ToTau = 6.907755;
constexpr double RetriggerFadeSeconds = 0.008;
constexpr double DriveCeiling = 0.7;
constexpr double OutputGain = 0.35;

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

SnarePmEngine::SnarePmEngine()
{
    m_snareFilterL.setMode(CascadedSvf::Mode::BandPass);
    m_snareFilterL.setOrder(2);
    m_snareFilterL.setResonance(0.35);

    m_snareFilterR.setMode(CascadedSvf::Mode::BandPass);
    m_snareFilterR.setOrder(2);
    m_snareFilterR.setResonance(0.35);

    m_shellResonator.setMode(CascadedSvf::Mode::BandPass);
    m_shellResonator.setOrder(2);
    m_shellResonator.setResonance(0.55);

    initModes();
    updateRates();
}

SnarePmEngine::~SnarePmEngine() = default;

void SnarePmEngine::initModes()
{
    // Batter head Bessel mode ratios: (0,1), (1,1), (2,1), (0,2), (3,1), (1,2), (4,1), (2,2)
    m_modes[0].ratio = 1.000;
    m_modes[1].ratio = 1.593;
    m_modes[2].ratio = 2.135;
    m_modes[3].ratio = 2.295;
    m_modes[4].ratio = 2.653;
    m_modes[5].ratio = 2.917;
    m_modes[6].ratio = 3.500;
    m_modes[7].ratio = 3.600;

    // Resonant bottom head modes coupled via drum shell air
    const double tension = static_cast<double>(m_headTension);
    m_modes[8].ratio = 1.000 * tension;
    m_modes[9].ratio = 1.593 * tension;
    m_modes[10].ratio = 2.135 * tension;
    m_modes[11].ratio = 2.295 * tension;

    // Modal amplitudes dependent on strike position (0.0 = center, 1.0 = rim),
    // stick tip hardness (0.0 = soft felt, 1.0 = hard nylon/wood tip),
    // and shell body weight (0.0 = lean piccolo, 1.0 = deep resonant shell).
    const double pos = static_cast<double>(std::clamp(m_position, 0.0f, 1.0f));
    const double hardness = static_cast<double>(std::clamp(m_hardness, 0.0f, 1.0f));
    const double body = static_cast<double>(std::clamp(m_body, 0.0f, 1.0f));
    const double overtoneScale = 0.25 + 0.75 * hardness;
    const double fundamentalScale = 0.35 + 1.0 * body;
    const double piHalf = std::numbers::pi * 0.5;

    m_modes[0].amplitude = std::cos(piHalf * pos) * fundamentalScale;
    m_modes[1].amplitude = 1.2 * std::sin(std::numbers::pi * pos) * overtoneScale;
    m_modes[2].amplitude = 1.0 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[3].amplitude = 0.8 * std::cos(piHalf * pos) * overtoneScale;
    m_modes[4].amplitude = 0.7 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[5].amplitude = 0.6 * std::sin(std::numbers::pi * pos) * overtoneScale;
    m_modes[6].amplitude = 0.5 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[7].amplitude = 0.4 * std::sin(piHalf * pos) * overtoneScale;

    // Bottom modes excitation via internal shell air coupling
    m_modes[8].amplitude = 0.35 * std::cos(piHalf * pos) * fundamentalScale;
    m_modes[9].amplitude = 0.25 * std::sin(std::numbers::pi * pos) * overtoneScale;
    m_modes[10].amplitude = 0.20 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[11].amplitude = 0.15 * std::cos(piHalf * pos) * overtoneScale;
}

void SnarePmEngine::updateModeDamping()
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }

    const double t0 = std::max(0.005, static_cast<double>(m_decay));
    const double tau0 = t0 / T60ToTau;
    const double toneDampingSlope = 1.0 + (1.0 - static_cast<double>(m_tone)) * 6.0;
    const double body = static_cast<double>(std::clamp(m_body, 0.0f, 1.0f));
    const double fundamentalDecayFactor = 0.4 + 1.0 * body;

    for (int i = 0; i < TotalModes; i++) {
        const double ratio = m_modes[i].ratio;
        double modeTau = tau0 / (1.0 + (ratio - 1.0) * toneDampingSlope);
        if (i == 0 || i == BatterModes) {
            modeTau *= fundamentalDecayFactor;
        }
        m_modes[i].damping = exponentialDecayPerSample(modeTau, sr);
    }
}

void SnarePmEngine::updateRates()
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }
    m_lastSampleRate = sr;

    initModes();
    updateModeDamping();

    const double pitchTau = std::max(0.001, static_cast<double>(m_pitchDecay)) / T60ToTau;
    m_pitchEnvDecayRate = exponentialDecayPerSample(pitchTau, sr);

    const double snareTau = std::max(0.01, static_cast<double>(m_snareDecay)) / T60ToTau;
    m_snareEnvDecayRate = exponentialDecayPerSample(snareTau, sr);

    const double buzzTime = static_cast<double>(m_snareDecay) * 1.5 + static_cast<double>(m_snareBuzz) * 0.45;
    const double buzzTau = std::max(0.01, buzzTime) / T60ToTau;
    m_buzzEnvDecayRate = exponentialDecayPerSample(buzzTau, sr);

    const double clickHz = 1800.0 + static_cast<double>(m_hardness) * 4200.0;
    m_clickPhaseIncrement = 2.0 * std::numbers::pi * clickHz / sr;
    const double clickTime = 0.0015 + (1.0 - static_cast<double>(m_hardness)) * 0.0055;
    m_clickDecayRate = exponentialDecayPerSample(clickTime / T60ToTau, sr);

    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds / T60ToTau, sr);

    // Snare wire filter bandpass frequency mapping based on tension (2200 Hz to 6500 Hz)
    const double snareHz = 2200.0 + static_cast<double>(m_snareTension) * 4300.0;
    const double snareCutoff = hzToNormalizedCutoff(snareHz, sr);
    m_snareFilterL.setSampleRate(sr);
    m_snareFilterR.setSampleRate(sr);
    m_snareFilterL.setCutoff(snareCutoff);
    m_snareFilterR.setCutoff(snareCutoff);

    // Shell cavity resonance filter tuned around fundamental
    const double body = static_cast<double>(std::clamp(m_body, 0.0f, 1.0f));
    const double shellHz = std::clamp(static_cast<double>(m_tune) * (1.35 - body * 0.40), 100.0, 500.0);
    const double shellResonance = 0.25 + body * 0.60;
    m_shellResonator.setSampleRate(sr);
    m_shellResonator.setCutoff(hzToNormalizedCutoff(shellHz, sr));
    m_shellResonator.setResonance(shellResonance);

    m_noiseBankL.setOversampleFactor(oversampleFactor());
    m_noiseBankR.setOversampleFactor(oversampleFactor());
}

void SnarePmEngine::trigger(float velocity)
{
    if (sampleRate() != m_lastSampleRate) {
        updateRates();
    }

    m_velocity = std::clamp(velocity, 0.0f, 1.0f);
    m_active = true;

    // Pitch envelope reset
    m_pitchEnv = 1.0;

    // Wire burst & buzz envelope reset
    m_snareEnv = 1.0;
    m_buzzEnv = 1.0;

    // Click transient reset
    m_clickSample = 1.0;
    m_clickPhase = 0.0;

    // Smooth choke / hand residual to tail phasors on retrigger
    triggerModes(m_velocity);
}

void SnarePmEngine::triggerModes(float velocity)
{
    const double vel = static_cast<double>(velocity);

    for (int i = 0; i < TotalModes; i++) {
        m_modes[i].tailRe += m_modes[i].re;
        m_modes[i].tailIm += m_modes[i].im;
        m_modes[i].re = vel * m_modes[i].amplitude;
        m_modes[i].im = 0.0;
    }
}

float SnarePmEngine::nextSample()
{
    float left = 0.0f;
    float right = 0.0f;
    nextSample(left, right);
    return (left + right) * 0.5f;
}

void SnarePmEngine::nextSample(float & left, float & right)
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

    // Dynamic pitch deflection from strike impact
    const double pitchRatio = std::exp2(m_pitchEnv * (static_cast<double>(m_pitchDepth) / 12.0));
    const double baseFreq = static_cast<double>(m_tune) * pitchRatio;

    // Membrane modal synthesis
    double batterHead = 0.0;
    double bottomHead = 0.0;
    double totalEnergy = 0.0;

    for (int i = 0; i < TotalModes; i++) {
        const double freq = std::clamp(baseFreq * m_modes[i].ratio, 20.0, sr * 0.48);
        const double omega = 2.0 * std::numbers::pi * freq / sr;
        const double cosOmega = std::cos(omega);
        const double sinOmega = std::sin(omega);

        // Active mode phasor
        const double re = m_modes[i].re;
        const double im = m_modes[i].im;
        const double d = m_modes[i].damping;
        m_modes[i].re = d * (re * cosOmega - im * sinOmega);
        m_modes[i].im = d * (re * sinOmega + im * cosOmega);

        // Retrigger tail phasor
        const double tre = m_modes[i].tailRe;
        const double tim = m_modes[i].tailIm;
        if (std::abs(tre) > 1.0e-7 || std::abs(tim) > 1.0e-7) {
            m_modes[i].tailRe = m_tailDamping * (tre * cosOmega - tim * sinOmega);
            m_modes[i].tailIm = m_tailDamping * (tre * sinOmega + tim * cosOmega);
        } else {
            m_modes[i].tailRe = 0.0;
            m_modes[i].tailIm = 0.0;
        }

        const double modeOut = m_modes[i].im + m_modes[i].tailIm;
        if (i < BatterModes) {
            batterHead += modeOut;
        } else {
            bottomHead += modeOut;
        }

        totalEnergy += std::abs(m_modes[i].re) + std::abs(m_modes[i].im) + std::abs(m_modes[i].tailRe) + std::abs(m_modes[i].tailIm);
    }

    // Stick strike click transient
    const double clickPulse = std::cos(m_clickPhase) * m_clickSample;
    const double click = clickPulse * (static_cast<double>(m_click) * 2.0) * static_cast<double>(m_velocity);
    m_clickPhase += m_clickPhaseIncrement;
    if (m_clickPhase > 2.0 * std::numbers::pi) {
        m_clickPhase -= 2.0 * std::numbers::pi;
    }
    m_clickSample *= m_clickDecayRate;

    // Shell cavity resonance
    const double shellInput = batterHead * 0.7 + click * 0.3;
    const double shellBody = m_shellResonator.process(shellInput);

    // Snare wire rattle & non-linear contact chatter
    if (m_noiseBankL.needsBaseSample()) {
        m_noiseBankL.setBaseSample(m_dist(m_rngL));
    }
    if (m_noiseBankR.needsBaseSample()) {
        m_noiseBankR.setBaseSample(m_dist(m_rngR));
    }

    const double buzzAmount = static_cast<double>(m_snareBuzz);
    const double excursion = std::clamp(std::abs(bottomHead), 0.0, 1.0);
    const double contactBuzz = excursion * (buzzAmount * 0.35);
    const double buzzTail = m_buzzEnv * (buzzAmount * 0.85);
    const double wireExcitation = (m_snareEnv + contactBuzz + buzzTail) * static_cast<double>(m_velocity);

    const double rawNoiseL = static_cast<double>(m_noiseBankL.nextSample());
    const double rawNoiseR = static_cast<double>(m_noiseBankR.nextSample());
    const double wireL = m_snareFilterL.process(rawNoiseL * wireExcitation);
    const double wireR = m_snareFilterR.process(rawNoiseR * wireExcitation);

    m_pitchEnv *= m_pitchEnvDecayRate;
    m_snareEnv *= m_snareEnvDecayRate;
    m_buzzEnv *= m_buzzEnvDecayRate;

    // Head fundamental and body remain centered for mono mix punch
    const double headSignal = batterHead + bottomHead * 0.35 + shellBody * (static_cast<double>(m_body) * 1.5) + click;

    // Pan Spread: stereo decorrelation of snare wires and rattle
    const double wireMid = (wireL + wireR) * 0.5;
    const double wireSide = (wireL - wireR) * 0.5;
    const double spread = static_cast<double>(m_panSpread);
    const double wireOutL = (wireMid + wireSide * spread) * static_cast<double>(m_snappy);
    const double wireOutR = (wireMid - wireSide * spread) * static_cast<double>(m_snappy);

    double outL = (headSignal + wireOutL) * OutputGain;
    double outR = (headSignal + wireOutR) * OutputGain;

    // Drive / Saturation stage
    if (m_drive > 0.0f) {
        const double gain = 1.0 + static_cast<double>(m_drive) * 8.0;
        const double norm = std::tanh(gain);
        const double satL = (std::tanh(outL * gain) / norm) * DriveCeiling;
        const double satR = (std::tanh(outR * gain) / norm) * DriveCeiling;
        const double driveFactor = static_cast<double>(m_drive);
        outL += (satL - outL) * driveFactor;
        outR += (satR - outR) * driveFactor;
    }

    left = static_cast<float>(outL);
    right = static_cast<float>(outR);

    // Energy tracking for voice active state
    totalEnergy += m_snareEnv + m_buzzEnv * buzzAmount + m_clickSample;
    if (totalEnergy < AmplitudeThreshold) {
        m_active = false;
        reset();
    }
}

bool SnarePmEngine::isActive() const
{
    return m_active;
}

void SnarePmEngine::reset()
{
    for (int i = 0; i < TotalModes; i++) {
        m_modes[i].re = 0.0;
        m_modes[i].im = 0.0;
        m_modes[i].tailRe = 0.0;
        m_modes[i].tailIm = 0.0;
    }

    m_pitchEnv = 0.0;
    m_snareEnv = 0.0;
    m_buzzEnv = 0.0;
    m_clickSample = 0.0;
    m_clickPhase = 0.0;

    m_snareFilterL.reset();
    m_snareFilterR.reset();
    m_shellResonator.reset();
    m_noiseBankL.reset();
    m_noiseBankR.reset();

    m_rngL.seed(RngSeedL);
    m_rngR.seed(RngSeedR);
}

void SnarePmEngine::stop()
{
    m_active = false;
    reset();
}

void SnarePmEngine::setTune(float tuneHz)
{
    m_tune = tuneHz;
    updateRates();
}

void SnarePmEngine::setTone(float tone)
{
    m_tone = tone;
    updateRates();
}

void SnarePmEngine::setDecay(float decaySeconds)
{
    m_decay = decaySeconds;
    updateRates();
}

void SnarePmEngine::setHeadTension(float ratio)
{
    m_headTension = ratio;
    updateRates();
}

void SnarePmEngine::setPitchDepth(float semitones)
{
    m_pitchDepth = semitones;
}

void SnarePmEngine::setPitchDecay(float seconds)
{
    m_pitchDecay = seconds;
    updateRates();
}

void SnarePmEngine::setSnappy(float snappy)
{
    m_snappy = snappy;
}

void SnarePmEngine::setSnareDecay(float seconds)
{
    m_snareDecay = seconds;
    updateRates();
}

void SnarePmEngine::setSnareTension(float tension)
{
    m_snareTension = tension;
    updateRates();
}

void SnarePmEngine::setSnareBuzz(float buzz)
{
    m_snareBuzz = buzz;
    updateRates();
}

void SnarePmEngine::setClick(float click)
{
    m_click = click;
}

void SnarePmEngine::setHardness(float hardness)
{
    m_hardness = hardness;
    updateRates();
}

void SnarePmEngine::setPosition(float position)
{
    m_position = position;
    initModes();
}

void SnarePmEngine::setBody(float body)
{
    m_body = body;
    updateRates();
}

void SnarePmEngine::setDrive(float drive)
{
    m_drive = drive;
}

void SnarePmEngine::setPanSpread(float spread)
{
    m_panSpread = spread;
}

} // namespace noteahead
