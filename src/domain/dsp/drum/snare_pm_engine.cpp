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

//! A Butterworth response from CascadedSvf: k = 2 * (1 - resonance) = sqrt(2)
constexpr double FlatResonance = 0.2929;

//! The crack holds for about 20 ms and is 8 dB down at 40 ms, as the 300 Hz-1.5 kHz band of a
//! hard hit on a real 14x8 snare is
constexpr double CrackTau = 0.042;
constexpr double CrackLowHz = 250.0;
constexpr double CrackHighHz = 2500.0;
constexpr double CrackGain = 1.30;

//! The wide wire band: the wires of a real snare reach down to 350 Hz under a hard hit
constexpr double WideWireLowHz = 350.0;
constexpr double WideWireGain = 1.0;
constexpr double WideWireBloomSeconds = 0.015;

//! The heads of a real 14x8 snare drop 11 dB from 20 to 40 ms after a hard hit and then ring on
constexpr double PunchTau = 0.015;
constexpr double PunchMaxGain = 4.0;

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
    m_snareFilterL.setResonance(0.20);

    m_snareFilterR.setMode(CascadedSvf::Mode::BandPass);
    m_snareFilterR.setOrder(2);
    m_snareFilterR.setResonance(0.20);

    m_clickFilter.setMode(CascadedSvf::Mode::BandPass);
    m_clickFilter.setOrder(2);
    m_clickFilter.setResonance(0.30);

    m_shellResonator.setMode(CascadedSvf::Mode::BandPass);
    m_shellResonator.setOrder(2);
    m_shellResonator.setResonance(0.55);

    for (auto * filter : { &m_wideHpL, &m_wideHpR, &m_crackHp }) {
        filter->setMode(CascadedSvf::Mode::HighPass);
        filter->setOrder(2);
        filter->setResonance(FlatResonance);
    }
    for (auto * filter : { &m_snareLpL, &m_snareLpR, &m_wideLpL, &m_wideLpR, &m_crackLp }) {
        filter->setMode(CascadedSvf::Mode::LowPass);
        filter->setOrder(2);
        filter->setResonance(FlatResonance);
    }
    m_crackLp.setOrder(4);

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
    // Lug/snare-bed asymmetry doublets, higher ring modes, and hoop/rim ping
    m_modes[8].ratio = 1.635;
    m_modes[9].ratio = 2.185;
    m_modes[10].ratio = 2.715;
    m_modes[11].ratio = 4.240;
    m_modes[12].ratio = 4.860;
    m_modes[13].ratio = 9.400;

    // Resonant bottom head modes coupled via drum shell air
    const double tension = static_cast<double>(m_headTension);
    m_modes[14].ratio = 1.000 * tension;
    m_modes[15].ratio = 1.593 * tension;
    m_modes[16].ratio = 2.135 * tension;
    m_modes[17].ratio = 2.295 * tension;

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

    m_modes[8].amplitude = 0.55 * m_modes[1].amplitude;
    m_modes[9].amplitude = 0.55 * m_modes[2].amplitude;
    m_modes[10].amplitude = 0.55 * m_modes[4].amplitude;
    m_modes[11].amplitude = 0.30 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[12].amplitude = 0.24 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[13].amplitude = 0.75 * (pos * pos) * (0.25 + 0.75 * hardness);

    // Bottom modes excitation via internal shell air coupling
    m_modes[14].amplitude = 0.35 * std::cos(piHalf * pos) * fundamentalScale;
    m_modes[15].amplitude = 0.30 * std::sin(std::numbers::pi * pos) * overtoneScale;
    m_modes[16].amplitude = 0.24 * std::sin(piHalf * pos) * overtoneScale;
    m_modes[17].amplitude = 0.18 * std::cos(piHalf * pos) * overtoneScale;
}

void SnarePmEngine::updateModeDamping()
{
    const double sr = sampleRate();
    if (sr <= 0.0) {
        return;
    }

    const double t0 = std::max(0.005, static_cast<double>(m_decay));
    const double tau0 = t0 / T60ToTau;
    const double tone = static_cast<double>(std::clamp(m_tone, 0.0f, 1.0f));
    const double body = static_cast<double>(std::clamp(m_body, 0.0f, 1.0f));
    const double toneRing = 0.25 + 1.05 * tone;
    const double slope = 0.35 + (1.0 - tone) * 2.2;

    for (int i = 0; i < TotalModes; i++) {
        const double ratio = m_modes[i].ratio;
        double modeTau = 0.0;
        if (i == 0 || i == BatterModes) {
            // (0,1) fundamental radiates strongly into air and responds to Body
            modeTau = tau0 * (0.45 + 0.90 * body);
        } else if (i == 1 || i == 2 || i == 4 || i == 8 || i == 9 || i == 10 || i == 15 || i == 16) {
            // Antisymmetric ring modes & doublets radiate poorly and ring longer at high Tone
            modeTau = tau0 * toneRing / (1.0 + 0.18 * (ratio - 1.0));
        } else if (i == 13) {
            // Hoop/rim ping
            modeTau = tau0 * (0.12 + 0.22 * tone);
        } else {
            modeTau = tau0 / (1.0 + (ratio - 1.0) * slope);
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

    const double hardness = static_cast<double>(std::clamp(m_hardness, 0.0f, 1.0f));
    const double clickFilterHz = 1200.0 + hardness * 3800.0;
    m_clickFilter.setSampleRate(sr);
    m_clickFilter.setCutoff(hzToNormalizedCutoff(clickFilterHz, sr));

    const double clickHz = 1400.0 + hardness * 2000.0;
    m_clickPhaseIncrement = 2.0 * std::numbers::pi * clickHz / sr;
    const double clickTime = 0.0020 + (1.0 - hardness) * 0.0045;
    m_clickDecayRate = exponentialDecayPerSample(clickTime / T60ToTau, sr);
    m_clickWoodDecayRate = exponentialDecayPerSample((clickTime * 0.45) / T60ToTau, sr);

    m_tailDamping = exponentialDecayPerSample(RetriggerFadeSeconds / T60ToTau, sr);

    // Snare wire filter bandpass & high-frequency roll-off based on tension
    const double tension = static_cast<double>(std::clamp(m_snareTension, 0.0f, 1.0f));
    const double snareHz = 1800.0 + tension * 3600.0;
    const double snareCutoff = hzToNormalizedCutoff(snareHz, sr);
    m_snareFilterL.setSampleRate(sr);
    m_snareFilterR.setSampleRate(sr);
    m_snareFilterL.setCutoff(snareCutoff);
    m_snareFilterR.setCutoff(snareCutoff);

    const double wireLpHz = 6500.0 + tension * 2500.0;
    const double wireLpCutoff = hzToNormalizedCutoff(wireLpHz, sr);
    for (auto * filter : { &m_snareLpL, &m_snareLpR, &m_wideLpL, &m_wideLpR }) {
        filter->setSampleRate(sr);
        filter->setCutoff(wireLpCutoff);
    }

    m_bottomDelaySamples = std::clamp<size_t>(static_cast<size_t>(std::lround(0.0005 * sr)), 1, m_bottomDelayBuf.size() - 1);
    m_bottomEnvAttackRate = 1.0 - exponentialDecayPerSample(0.0005, sr);
    m_bottomEnvReleaseRate = 1.0 - exponentialDecayPerSample(0.008, sr);

    m_wireCombDelayL = std::clamp<size_t>(static_cast<size_t>(std::lround(0.00041 * sr)), 1, m_wireCombL.size() - 1);
    m_wireCombDelayR = std::clamp<size_t>(static_cast<size_t>(std::lround(0.00053 * sr)), 1, m_wireCombR.size() - 1);

    // Shell cavity resonance filter tuned around fundamental
    const double body = static_cast<double>(std::clamp(m_body, 0.0f, 1.0f));
    const double shellHz = std::clamp(static_cast<double>(m_tune) * (1.35 - body * 0.40), 100.0, 500.0);
    const double shellResonance = 0.25 + body * 0.60;
    m_shellResonator.setSampleRate(sr);
    m_shellResonator.setCutoff(hzToNormalizedCutoff(shellHz, sr));
    m_shellResonator.setResonance(shellResonance);

    for (auto * filter : { &m_wideHpL, &m_wideHpR }) {
        filter->setSampleRate(sr);
        filter->setCutoff(hzToNormalizedCutoff(WideWireLowHz, sr));
    }
    m_wideBloomCoefficient = 1.0 - std::exp(-1.0 / (WideWireBloomSeconds * sr));
    m_punchDecayRate = exponentialDecayPerSample(PunchTau, sr);
    m_crackHp.setSampleRate(sr);
    m_crackHp.setCutoff(hzToNormalizedCutoff(CrackLowHz, sr));
    m_crackLp.setSampleRate(sr);
    m_crackLp.setCutoff(hzToNormalizedCutoff(CrackHighHz, sr));

    // exp(-(n / (tau * sr))^2): each sample's ratio to the last is exp(-(2n + 1) * a)
    const double crackA = 1.0 / std::pow(CrackTau * sr, 2.0);
    m_crackFirstRatio = std::exp(-crackA);
    m_crackRatioStep = std::exp(-2.0 * crackA);

    m_noiseBankL.setOversampleFactor(oversampleFactor());
    m_noiseBankR.setOversampleFactor(oversampleFactor());
    m_noiseBankCrack.setOversampleFactor(oversampleFactor());
    m_noiseBankClick.setOversampleFactor(oversampleFactor());
}

void SnarePmEngine::trigger(float velocity)
{
    if (sampleRate() != m_lastSampleRate) {
        updateRates();
    }

    m_velocity = std::clamp(velocity, 0.0f, 1.0f);
    const double vel = static_cast<double>(m_velocity);
    m_velPitchScale = 0.25 + 0.75 * vel;
    m_velStrikeScale = std::pow(vel, 1.35);
    m_velWireScale = std::pow(vel, 1.15);
    m_active = true;

    // Pitch envelope reset
    m_pitchEnv = 1.0;

    // Wire burst & buzz envelope reset
    m_snareEnv = 1.0;
    m_buzzEnv = 1.0;

    // Click transient reset
    m_clickSample = 1.0;
    m_clickWoodEnv = 1.0;
    m_clickPhase = 0.0;

    m_punchEnv = 1.0;

    // Crack envelope reset
    m_crackEnv = 1.0;
    m_crackRatio = m_crackFirstRatio;

    // Smooth choke / hand residual to tail phasors on retrigger
    triggerModes(m_velocity);
}

void SnarePmEngine::triggerModes(float velocity)
{
    static constexpr std::array<double, TotalModes> modePhases {
        0.0, 0.75, 1.40, 2.15, 0.55, 1.95, 2.60, 0.95,
        2.35, 3.05, 1.85, 0.40, 2.75, 1.15,
        0.0, 1.25, 2.10, 0.65
    };

    const double vel = static_cast<double>(velocity);
    const double upperVelScale = vel * (0.45 + 0.55 * std::pow(vel, 0.75));

    for (int i = 0; i < TotalModes; i++) {
        m_modes[i].tailRe += m_modes[i].re;
        m_modes[i].tailIm += m_modes[i].im;
        const double amp = (m_modes[i].ratio > 1.2 ? upperVelScale : vel) * m_modes[i].amplitude;
        m_modes[i].re = amp * std::cos(modePhases[i]);
        m_modes[i].im = amp * std::sin(modePhases[i]);
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

    // Dynamic pitch deflection from strike impact (scales nonlinearly with velocity)
    const double pitchRatio = std::exp2(m_pitchEnv * (static_cast<double>(m_pitchDepth) * m_velPitchScale / 12.0));
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

    const double noiseDensity = std::sqrt(baseSampleRate() / 44100.0);

    // Stick strike click transient (broadband stick-tip impact + brief wooden body resonance)
    if (m_noiseBankClick.needsBaseSample()) {
        m_noiseBankClick.setBaseSample(m_dist(m_rngClick));
    }
    const double rawClickNoise = static_cast<double>(m_noiseBankClick.nextSample()) * noiseDensity;
    const double clickNoise = m_clickFilter.process(rawClickNoise) * m_clickSample;
    const double clickWood = std::sin(m_clickPhase) * m_clickWoodEnv;
    const double click = (clickNoise * 2.1 + clickWood * 0.55) * (static_cast<double>(m_click) * 2.0) * m_velStrikeScale;
    m_clickPhase += m_clickPhaseIncrement;
    if (m_clickPhase > 2.0 * std::numbers::pi) {
        m_clickPhase -= 2.0 * std::numbers::pi;
    }
    m_clickSample *= m_clickDecayRate;
    m_clickWoodEnv *= m_clickWoodDecayRate;

    // Stick crack: band-limited noise, centred with the head
    double crack = 0.0;
    if (m_crack > 0.0f && m_crackEnv > 1.0e-6) {
        if (m_noiseBankCrack.needsBaseSample()) {
            m_noiseBankCrack.setBaseSample(m_dist(m_rngCrack));
        }
        const double noise = static_cast<double>(m_noiseBankCrack.nextSample()) * noiseDensity;
        crack = m_crackLp.process(m_crackHp.process(noise)) * m_crackEnv * static_cast<double>(m_crack) * CrackGain * m_velStrikeScale;
        m_crackEnv *= m_crackRatio;
        m_crackRatio *= m_crackRatioStep;
    }

    // Shell cavity resonance
    const double shellInput = batterHead * 0.7 + click * 0.3;
    const double shellBody = m_shellResonator.process(shellInput);

    // Snare wire rattle & non-linear contact chatter coupled via acoustic shell-transit delay
    if (m_noiseBankL.needsBaseSample()) {
        m_noiseBankL.setBaseSample(m_dist(m_rngL));
    }
    if (m_noiseBankR.needsBaseSample()) {
        m_noiseBankR.setBaseSample(m_dist(m_rngR));
    }

    const size_t delaySize = m_bottomDelayBuf.size();
    const double delayedBottom = m_bottomDelayBuf[(m_bottomDelayPos + delaySize - m_bottomDelaySamples) % delaySize];
    m_bottomDelayBuf[m_bottomDelayPos] = bottomHead;
    m_bottomDelayPos = (m_bottomDelayPos + 1) % delaySize;

    const double absBottom = std::abs(delayedBottom);
    const double envCoeff = absBottom > m_bottomEnv ? m_bottomEnvAttackRate : m_bottomEnvReleaseRate;
    m_bottomEnv += (absBottom - m_bottomEnv) * envCoeff;

    const double buzzAmount = static_cast<double>(m_snareBuzz);
    const double excursion = std::clamp(absBottom, 0.0, 1.5);
    const double rattleMod = 1.0 + buzzAmount * 0.55 * std::tanh(excursion * 2.5);
    const double contactBuzz = (excursion * 0.45 + m_bottomEnv * 0.35) * buzzAmount;
    const double buzzTail = m_buzzEnv * (buzzAmount * 0.85);
    const double wireExcitation = (m_snareEnv * rattleMod + contactBuzz + buzzTail) * m_velWireScale;

    const double rawNoiseL = static_cast<double>(m_noiseBankL.nextSample()) * noiseDensity;
    const double rawNoiseR = static_cast<double>(m_noiseBankR.nextSample()) * noiseDensity;
    double wireL = m_snareLpL.process(m_snareFilterL.process(rawNoiseL * wireExcitation));
    double wireR = m_snareLpR.process(m_snareFilterR.process(rawNoiseR * wireExcitation));
    if (m_wireWidth > 0.0f) {
        const double width = static_cast<double>(m_wireWidth);
        m_wideExcitation += (wireExcitation - m_wideExcitation) * m_wideBloomCoefficient;
        const double wideL = m_wideLpL.process(m_wideHpL.process(rawNoiseL * m_wideExcitation)) * WideWireGain;
        const double wideR = m_wideLpR.process(m_wideHpR.process(rawNoiseR * m_wideExcitation)) * WideWireGain;
        wireL += (wideL - wireL) * width;
        wireR += (wideR - wireR) * width;
    }

    const size_t combSize = m_wireCombL.size();
    const double combOutL = wireL + 0.26 * m_wireCombL[(m_wireCombPos + combSize - m_wireCombDelayL) % combSize];
    const double combOutR = wireR + 0.26 * m_wireCombR[(m_wireCombPos + combSize - m_wireCombDelayR) % combSize];
    m_wireCombL[m_wireCombPos] = combOutL;
    m_wireCombR[m_wireCombPos] = combOutR;
    m_wireCombPos = (m_wireCombPos + 1) % combSize;
    wireL = wireL * 0.65 + combOutL * 0.35;
    wireR = wireR * 0.65 + combOutR * 0.35;

    m_pitchEnv *= m_pitchEnvDecayRate;
    m_snareEnv *= m_snareEnvDecayRate;
    m_buzzEnv *= m_buzzEnvDecayRate;

    // Head fundamental and body remain centered for mono mix punch
    if (m_punch > 0.0f) {
        const double punchGain = 1.0 + m_punchEnv * static_cast<double>(m_punch) * PunchMaxGain;
        batterHead *= punchGain;
        bottomHead *= punchGain;
        m_punchEnv *= m_punchDecayRate;
    }

    const double headSignal = batterHead + bottomHead * 0.35 + shellBody * (static_cast<double>(m_body) * 1.5) + click + crack;

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
    totalEnergy += m_snareEnv + m_buzzEnv * buzzAmount + m_clickSample + m_clickWoodEnv + (m_crack > 0.0f ? m_crackEnv : 0.0);
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
    m_clickWoodEnv = 0.0;
    m_clickPhase = 0.0;
    m_crackEnv = 0.0;
    m_wideExcitation = 0.0;
    m_punchEnv = 0.0;
    m_bottomEnv = 0.0;
    m_bottomDelayPos = 0;
    m_bottomDelayBuf.fill(0.0);
    m_wireCombPos = 0;
    m_wireCombL.fill(0.0);
    m_wireCombR.fill(0.0);

    m_snareFilterL.reset();
    m_snareFilterR.reset();
    m_snareLpL.reset();
    m_snareLpR.reset();
    m_clickFilter.reset();
    m_shellResonator.reset();
    m_wideHpL.reset();
    m_wideHpR.reset();
    m_wideLpL.reset();
    m_wideLpR.reset();
    m_crackHp.reset();
    m_crackLp.reset();
    m_noiseBankL.reset();
    m_noiseBankR.reset();
    m_noiseBankCrack.reset();
    m_noiseBankClick.reset();

    m_rngL.seed(RngSeedL);
    m_rngR.seed(RngSeedR);
    m_rngCrack.seed(RngSeedCrack);
    m_rngClick.seed(RngSeedClick);
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

void SnarePmEngine::setCrack(float crack)
{
    m_crack = crack;
}

void SnarePmEngine::setWireWidth(float width)
{
    m_wireWidth = width;
}

void SnarePmEngine::setPunch(float punch)
{
    m_punch = punch;
}

} // namespace noteahead
