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
//! Trims default Init peak to about -5 dBFS
constexpr double LevelTrim = 0.862;

//! A Butterworth response from CascadedSvf: k = 2 * (1 - resonance) = sqrt(2)
constexpr double FlatResonance = 0.2929;

//! Primary batter head modes: air stiffness inside the shell lifts the first radial/azimuthal
//! modes relative to a free membrane, while higher ring and radial modes reach past 5.5x.
constexpr std::array<double, 10> PrimaryBatterRatios { 1.000, 1.680, 2.160, 2.420, 2.720, 3.280, 3.880, 4.360, 5.050, 5.780 };
constexpr std::array<double, 10> PrimaryBatterLevels { 1.00, 0.78, 0.68, 0.56, 0.52, 0.42, 0.36, 0.34, 0.26, 0.22 };
constexpr std::array<double, 4> DoubletBatterRatios { 1.735, 2.230, 2.805, 4.490 };

constexpr std::array<double, KickPmEngine::FrontModes> FrontRatios { 1.000, 1.593, 2.135, 2.653, 3.500, 4.240 };
constexpr std::array<double, KickPmEngine::FrontModes> FrontLevels { 0.80, 0.50, 0.36, 0.25, 0.18, 0.12 };

//! Peak pitch rise of the batter head under quadratic geometric tension at full Pitch Drop
constexpr double MaxPitchRise = 0.75;
constexpr double FrontPitchRiseShare = 0.45;

//! Beater contact time, felt to hard at full velocity
constexpr double FeltContactSeconds = 0.006;
constexpr double HardContactSeconds = 0.0006;

constexpr double ClickLowHz = 650.0;

double exponentialDecayPerSample(double tau, double sampleRate)
{
    return std::exp(-1.0 / (std::max(tau, 1.0e-6) * sampleRate));
}

double hzToNormalizedCutoff(double hz, double sampleRate)
{
    const double maxFreq = std::min(20000.0, sampleRate * 0.49);
    return std::clamp(std::log2(std::max(20.0, hz) / 20.0) / std::log2(maxFreq / 20.0), 0.0, 1.0);
}

bool isBatterRingMode(int index)
{
    return index == 1 || index == 2 || index == 4 || index == 6 || index == 7 || index == 8 || index >= 10;
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

    m_thudFilter.setMode(CascadedSvf::Mode::BandPass);
    m_thudFilter.setOrder(2);
    m_thudFilter.setResonance(0.30);

    m_ringHp.setMode(CascadedSvf::Mode::HighPass);
    m_ringHp.setOrder(2);
    m_ringHp.setResonance(FlatResonance);
    m_ringLp.setMode(CascadedSvf::Mode::LowPass);
    m_ringLp.setOrder(4);
    m_ringLp.setResonance(FlatResonance);

    m_shellResonator.setMode(CascadedSvf::Mode::BandPass);
    m_shellResonator.setOrder(2);
    m_shellResonator.setResonance(0.6);

    initModes();
    updateRates();
}

KickPmEngine::~KickPmEngine() = default;

double KickPmEngine::contactSeconds(double velocity) const
{
    const double beater = static_cast<double>(std::clamp(m_beater, 0.0f, 1.0f));
    const double vel = std::clamp(velocity, 0.1, 1.0);
    return FeltContactSeconds * std::pow(HardContactSeconds / FeltContactSeconds, beater) * std::pow(vel, -0.35);
}

double KickPmEngine::contactWeight(double hz, double velocity) const
{
    // Spectrum of a half-sine force pulse of duration T, relative to DC:
    // cos(pi f T) / (1 - (2 f T)^2), equal to pi / 4 where denominator vanishes
    const double tc = contactSeconds(velocity);
    const double x = 2.0 * hz * tc;
    const double denominator = 1.0 - x * x;
    if (std::abs(denominator) < 1.0e-3) {
        return std::numbers::pi / 4.0;
    }
    return std::abs(std::cos(std::numbers::pi * hz * tc) / denominator);
}

void KickPmEngine::initModes()
{
    const double pos = static_cast<double>(std::clamp(m_position, 0.0f, 1.0f));
    const double front = static_cast<double>(std::clamp(m_frontHead, 0.0f, 1.0f));
    const double outside = static_cast<double>(std::clamp(m_mic, 0.0f, 1.0f));
    const double tune = static_cast<double>(m_tune);
    const double piHalf = std::numbers::pi * 0.5;

    for (size_t i = 0; i < PrimaryBatterRatios.size(); i++) {
        m_modes[i].ratio = PrimaryBatterRatios[i];
        double placement = 0.0;
        if (i == 0) {
            placement = 1.0 - 0.35 * pos;
        } else if (i == 3) {
            placement = 0.35 + 0.50 * std::cos(piHalf * pos);
        } else {
            placement = 0.16 + 0.96 * std::sin(piHalf * pos);
        }
        const double micWeight = i == 0 ? 1.0 : 1.0 - 0.42 * outside;
        m_modes[i].amplitude = PrimaryBatterLevels[i] * placement * micWeight * contactWeight(tune * m_modes[i].ratio, 1.0);
    }

    // Lug/hoop asymmetry mode doublets paired to (1,1), (2,1), (3,1) and (4,1)/(3,2)
    static constexpr std::array<int, 4> DoubletParents { 1, 2, 4, 7 };
    static constexpr std::array<double, 4> DoubletScales { 0.45, 0.45, 0.40, 0.40 };
    for (size_t i = 0; i < DoubletBatterRatios.size(); i++) {
        const int modeIdx = 10 + static_cast<int>(i);
        m_modes[modeIdx].ratio = DoubletBatterRatios[i];
        m_modes[modeIdx].amplitude = DoubletScales[i] * m_modes[DoubletParents[i]].amplitude;
    }

    const double frontLevel = (0.08 + 0.92 * front) * (0.4 + 0.6 * outside);
    for (int i = 0; i < FrontModes; i++) {
        auto & mode = m_modes[BatterModes + i];
        mode.ratio = FrontRatios[i] * static_cast<double>(m_frontTune);
        mode.amplitude = FrontLevels[i] * frontLevel * contactWeight(tune * FrontRatios[i], 1.0);
    }
    m_referenceAmplitude = std::max(1.0e-6, m_modes[0].amplitude);
}

void KickPmEngine::updateVelocityFilters(double velocity)
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }
    const double vel = std::clamp(velocity, 0.0, 1.0);
    const double beater = static_cast<double>(std::clamp(m_beater, 0.0f, 1.0f));
    const double tone = static_cast<double>(std::clamp(m_tone, 0.0f, 1.0f));
    const double clickHighHz = (2800.0 + 8500.0 * beater) * (0.35 + 0.65 * vel);
    m_clickLp.setCutoff(hzToNormalizedCutoff(clickHighHz, sr));
    const double ringHighHz = (600.0 + 2600.0 * tone) * (0.50 + 0.50 * vel);
    m_ringLp.setCutoff(hzToNormalizedCutoff(ringHighHz, sr));
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
    const double tone = static_cast<double>(std::clamp(m_tone, 0.0f, 1.0f));
    const double damp = static_cast<double>(std::clamp(m_damping, 0.0f, 1.0f));
    const double front = static_cast<double>(std::clamp(m_frontHead, 0.0f, 1.0f));

    const double dampCurve = damp * damp * 0.6 + damp * 0.4;
    const double batterDampFactor = std::pow(0.065, dampCurve);
    const double frontDampFactor = std::pow(0.14, damp * damp * 0.55 + damp * 0.45);
    const double toneRing = 0.20 + 1.15 * tone;
    const double slope = 0.35 + (1.0 - tone) * 2.4;
    const double frontDecayScale = (0.20 + 2.2 * front) * frontDampFactor;
    const double frontSlope = 0.45 + (1.0 - tone) * 2.0;

    for (int i = 0; i < TotalModes; i++) {
        double tau = tau0;
        if (i < BatterModes) {
            const double ratio = m_modes[i].ratio;
            if (i == 0) {
                tau = tau0 * batterDampFactor;
            } else if (isBatterRingMode(i)) {
                tau = tau0 * batterDampFactor * toneRing / (1.0 + 0.16 * (ratio - 1.0));
            } else {
                tau = tau0 * batterDampFactor / (1.0 + (ratio - 1.0) * slope);
            }
        } else {
            const double ratio = FrontRatios[i - BatterModes];
            tau = tau0 * frontDecayScale / (1.0 + (ratio - 1.0) * frontSlope);
        }
        m_modes[i].damping = exponentialDecayPerSample(tau, sr);
    }

    // Two-stage impact punch + fast tension relaxation
    m_impactAmount = 0.25 + 0.75 * damp + 0.35 * static_cast<double>(std::clamp(m_pitchDrop, 0.0f, 1.0f));
    m_impactDecayRate = exponentialDecayPerSample(0.024, sr);
    m_tensionDecayRate = exponentialDecayPerSample(0.045, sr);

    m_frontDelaySamples = std::clamp<size_t>(static_cast<size_t>(std::lround(0.0012 * sr)), 1, m_frontDelayBuf.size() - 1);

    // Beater high slap & low-mid thud
    const double beater = static_cast<double>(std::clamp(m_beater, 0.0f, 1.0f));
    const double clickTau = 0.011 - 0.0085 * beater;
    m_clickDecayRate = exponentialDecayPerSample(clickTau, sr);
    m_clickHp.setSampleRate(sr);
    m_clickHp.setCutoff(hzToNormalizedCutoff(ClickLowHz, sr));
    m_clickLp.setSampleRate(sr);

    const double thudTau = 0.015 - 0.007 * beater;
    m_thudDecayRate = exponentialDecayPerSample(thudTau, sr);
    m_thudFilter.setSampleRate(sr);
    m_thudFilter.setCutoff(hzToNormalizedCutoff(220.0 + 280.0 * beater, sr));

    // Shell resonance sits several times above the fundamental
    const double shellHz = std::clamp(static_cast<double>(m_tune) * 4.5, 120.0, 600.0);
    m_shellResonator.setSampleRate(sr);
    m_shellResonator.setCutoff(hzToNormalizedCutoff(shellHz, sr));

    // Upper-mode ring: exponential decay with pillow sensitivity and skin micro-comb
    const double ringDampScale = 1.0 / (1.0 + 1.8 * damp);
    m_ringDecayRate = exponentialDecayPerSample(std::max(0.01, static_cast<double>(m_ringDecay) * ringDampScale) / T60ToTau, sr);
    m_ringHp.setSampleRate(sr);
    m_ringHp.setCutoff(hzToNormalizedCutoff(std::max(160.0, static_cast<double>(m_tune) * 3.5), sr));
    m_ringLp.setSampleRate(sr);
    updateVelocityFilters(static_cast<double>(m_velocity));
    m_ringCombDelay = std::clamp<size_t>(static_cast<size_t>(std::lround(0.0017 * sr)), 1, m_ringCombBuf.size() - 1);

    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds / T60ToTau, sr);
    m_noiseBankClick.setOversampleFactor(oversampleFactor());
    m_noiseBankRing.setOversampleFactor(oversampleFactor());
}

void KickPmEngine::trigger(float velocity)
{
    if (sampleRate() != m_lastSampleRate) {
        updateRates();
    }
    const double sr = sampleRate();
    m_velocity = std::clamp(velocity, 0.0f, 1.0f);
    const double vel = static_cast<double>(m_velocity);
    updateVelocityFilters(vel);
    m_active = true;

    // Transfer current sounding level to tail before setting new strike envelopes
    triggerModes(m_velocity);

    const double tc = contactSeconds(vel);
    m_headAttack = 0.0;
    m_headAttackCoeff = 1.0 - exponentialDecayPerSample(std::max(0.00025, 0.35 * tc), sr);
    m_frontAttack = 0.0;
    m_frontAttackCoeff = 1.0 - exponentialDecayPerSample(0.0025, sr);

    m_impactEnv = vel > 0.0 ? 1.0 : 0.0;
    m_tensionEnv = 1.0;

    m_clickEnv = std::pow(vel, 2.20);
    m_thudEnv = std::pow(vel, 1.65);
    m_ringEnv = std::pow(vel, 1.80);
}

void KickPmEngine::triggerModes(float velocity)
{
    static constexpr std::array<double, TotalModes> modePhases {
        0.0, 0.85, 1.70, 2.35, 0.55, 2.05, 1.20, 2.75, 0.40, 1.55, 2.45, 0.95, 2.95, 1.85,
        0.0, 1.10, 2.25, 0.65, 2.80, 1.50
    };

    const double vel = static_cast<double>(velocity);
    const double upperVelScale = vel * (0.25 + 0.75 * vel);
    const double tune = static_cast<double>(m_tune);
    const double batterGain = (1.0 + m_impactAmount * m_impactEnv) * m_headAttack;

    for (int i = 0; i < TotalModes; i++) {
        auto & mode = m_modes[i];
        const double gain = i < BatterModes ? batterGain : m_frontAttack;
        mode.tailRe += mode.re * gain;
        mode.tailIm += mode.im * gain;

        const bool isFundamental = i == 0 || i == BatterModes;
        const double baseScale = isFundamental ? vel : upperVelScale;
        const double freq = tune * (i < BatterModes ? mode.ratio : FrontRatios[i - BatterModes]);
        const double refWeight = std::max(contactWeight(freq, 1.0), 1.0e-4);
        const double hertzScale = std::clamp(contactWeight(freq, vel) / refWeight, 0.15, 1.25);
        const double amp = baseScale * hertzScale * mode.amplitude;

        mode.re = amp * std::cos(modePhases[i]);
        mode.im = amp * std::sin(modePhases[i]);
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

    // Geometric membrane tension rises with displacement squared and relaxes quickly
    const double displacement = std::min(std::hypot(m_modes[0].re, m_modes[0].im) / m_referenceAmplitude, 1.5);
    const double tension = displacement * displacement * (0.45 + 0.55 * m_tensionEnv);
    const double rise = MaxPitchRise * static_cast<double>(m_pitchDrop) * tension;
    const double tune = static_cast<double>(m_tune);

    m_headAttack += (1.0 - m_headAttack) * m_headAttackCoeff;
    m_frontAttack += (1.0 - m_frontAttack) * m_frontAttackCoeff;
    const double batterGain = (1.0 + m_impactAmount * m_impactEnv) * m_headAttack;

    double batterActive = 0.0;
    double batterTail = 0.0;
    double frontActive = 0.0;
    double frontTail = 0.0;
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

        if (isFront) {
            frontActive += mode.im;
            frontTail += mode.tailIm;
            totalEnergy += (std::abs(mode.re) + std::abs(mode.im)) * m_frontAttack + std::abs(mode.tailRe) + std::abs(mode.tailIm);
        } else {
            batterActive += mode.im;
            batterTail += mode.tailIm;
            totalEnergy += (std::abs(mode.re) + std::abs(mode.im)) * batterGain + std::abs(mode.tailRe) + std::abs(mode.tailIm);
        }
    }

    const double batterHead = batterActive * batterGain + batterTail;
    const double rawFrontHead = frontActive * m_frontAttack + frontTail;

    // Acoustic shell-transit delay from batter strike to front head radiation
    const size_t delaySize = m_frontDelayBuf.size();
    const double frontHead = m_frontDelayBuf[(m_frontDelayPos + delaySize - m_frontDelaySamples) % delaySize];
    m_frontDelayBuf[m_frontDelayPos] = rawFrontHead;
    m_frontDelayPos = (m_frontDelayPos + 1) % delaySize;

    m_impactEnv *= m_impactDecayRate;
    m_tensionEnv *= m_tensionDecayRate;

    const double noiseDensity = std::sqrt(baseSampleRate() / 44100.0);

    double click = 0.0;
    if (m_click > 0.0f && (m_clickEnv > 1.0e-6 || m_thudEnv > 1.0e-6)) {
        if (m_noiseBankClick.needsBaseSample()) {
            m_noiseBankClick.setBaseSample(m_dist(m_rngClick));
        }
        const double noise = static_cast<double>(m_noiseBankClick.nextSample()) * noiseDensity;
        const double slap = m_clickLp.process(m_clickHp.process(noise)) * m_clickEnv;
        const double thud = m_thudFilter.process(noise) * m_thudEnv;
        const double slapMicWeight = 1.0 - 0.65 * static_cast<double>(m_mic);
        const double thudMicWeight = 1.0 - 0.50 * static_cast<double>(m_mic);
        click = (slap * slapMicWeight * 1.15 + thud * thudMicWeight * 0.55) * static_cast<double>(m_click);
        m_clickEnv *= m_clickDecayRate;
        m_thudEnv *= m_thudDecayRate;
    }

    double ring = 0.0;
    if (m_ring > 0.0f && m_ringEnv > 1.0e-6) {
        if (m_noiseBankRing.needsBaseSample()) {
            m_noiseBankRing.setBaseSample(m_dist(m_rngRing));
        }
        const double noise = static_cast<double>(m_noiseBankRing.nextSample()) * noiseDensity;
        const double ringFiltered = m_ringLp.process(m_ringHp.process(noise));
        const size_t combSize = m_ringCombBuf.size();
        const double combOut = ringFiltered + 0.28 * m_ringCombBuf[(m_ringCombPos + combSize - m_ringCombDelay) % combSize];
        m_ringCombBuf[m_ringCombPos] = combOut;
        m_ringCombPos = (m_ringCombPos + 1) % combSize;
        ring = (ringFiltered * 0.70 + combOut * 0.30) * m_ringEnv * static_cast<double>(m_ring);
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

    totalEnergy += (m_click > 0.0f ? m_clickEnv + m_thudEnv : 0.0) + (m_ring > 0.0f ? m_ringEnv : 0.0);
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
    m_impactEnv = 0.0;
    m_tensionEnv = 0.0;
    m_headAttack = 1.0;
    m_frontAttack = 1.0;
    m_frontDelayPos = 0;
    m_frontDelayBuf.fill(0.0);
    m_ringCombPos = 0;
    m_ringCombBuf.fill(0.0);
    m_clickEnv = 0.0;
    m_thudEnv = 0.0;
    m_ringEnv = 0.0;
    m_ringHp.reset();
    m_ringLp.reset();
    m_clickHp.reset();
    m_clickLp.reset();
    m_thudFilter.reset();
    m_shellResonator.reset();
    m_noiseBankClick.reset();
    m_noiseBankRing.reset();
    m_rngClick.seed(RngSeedClick);
    m_rngRing.seed(RngSeedRing);
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
    updateRates();
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
