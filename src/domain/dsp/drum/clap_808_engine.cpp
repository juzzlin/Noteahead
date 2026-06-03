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

#include "clap_808_engine.hpp"

#include <cmath>

namespace noteahead {

namespace {

// The RD-8's clap at noon, fitted against RD-8 on third octaves within 2.1 dB
constexpr double ClapHz = 1621.17;
constexpr double ClapQ = 1.794;
constexpr double ClapLowPassHz = 6737.33;
constexpr int Bursts = 3;
constexpr double BurstGapSeconds = 0.00878709;
constexpr double BurstSeconds = 0.00424493;
constexpr double TailLevel = 1.93151;
constexpr double TailStartSeconds = 0.0293142;
constexpr double TailAttackSeconds = 0.00100701;
//! The tail is two decays, a fast one that most of it is in and a slow one under it
constexpr double TailFast = 0.943874;
constexpr double TailFastSeconds = 0.0150286;
constexpr double TailSeconds = 0.0162349;
constexpr double ClapGain = 1.1316;

//! Offset on the RD-8, against noon, fitted on a sweep within 3.0 to 4.0 dB: it is as much the
//! clap's level as its length, nearly silent at zero
constexpr Drum808::KnobCurve OffsetLevel { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.109, 0.333, 1.0, 1.530, 1.301 } };
constexpr Drum808::KnobCurve OffsetBurstTime { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.793, 0.741, 1.0, 1.540, 2.378 } };
constexpr Drum808::KnobCurve OffsetTailLevel { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 1.164, 0.895, 1.0, 0.886, 1.251 } };
constexpr Drum808::KnobCurve OffsetTailTime { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.654, 0.692, 1.0, 1.735, 1.976 } };

// The RD-8's maracas, fitted against RD-8 within 3.6 dB. Offset is not measured on them,
// so it leaves them alone.
constexpr double MaracasHighPassHz = 3112.15;
constexpr double MaracasLowPassHz = 10136.62;
constexpr double MaracasAttackSeconds = 0.00491444;
constexpr double MaracasHoldSeconds = 0.0204146;
constexpr double MaracasReleaseSeconds = 0.00221164;
constexpr double MaracasGain = 0.3677;

constexpr double ButterworthQ = 0.70710678;
constexpr double SilenceLevel = 1.0e-4;

} // namespace

Clap808Engine::Clap808Engine() = default;

void Clap808Engine::setOffset(float offset)
{
    m_offset = offset;
}

void Clap808Engine::setMaracas(bool maracas)
{
    m_maracas = maracas;
}

void Clap808Engine::trigger(float velocity)
{
    const double rate = sampleRate();
    m_active = true;
    m_time = 0.0;
    m_velocity = static_cast<double>(velocity);
    m_level = OffsetLevel.ratioAt(m_offset);
    m_burstScale = OffsetBurstTime.ratioAt(m_offset);
    m_tailLevel = OffsetTailLevel.ratioAt(m_offset);
    m_tailScale = OffsetTailTime.ratioAt(m_offset);
    m_burstNoise.reseed();
    m_tailNoise.reseed();
    m_burstBand.setBandPass(ClapHz, ClapQ, rate);
    m_burstLowPass.setLowPass(ClapLowPassHz, ButterworthQ, rate);
    m_tailBand.setBandPass(ClapHz, ClapQ, rate);
    m_tailLowPass.setLowPass(ClapLowPassHz, ButterworthQ, rate);
    m_highPass1.setHighPass(MaracasHighPassHz, ButterworthQ, rate);
    m_highPass2.setHighPass(MaracasHighPassHz, ButterworthQ, rate);
    m_lowPass.setLowPass(MaracasLowPassHz, ButterworthQ, rate);
    for (auto * filter : { &m_burstBand, &m_burstLowPass, &m_tailBand, &m_tailLowPass, &m_highPass1, &m_highPass2, &m_lowPass }) {
        filter->reset();
    }
}

double Clap808Engine::clapSample(double t)
{
    double bursts = 0.0;
    for (int i = 0; i < Bursts; i++) {
        if (const double start = i * BurstGapSeconds; t >= start) {
            bursts += std::exp(-(t - start) / (BurstSeconds * m_burstScale));
        }
    }
    double y = bursts * m_burstLowPass.process(m_burstBand.process(m_burstNoise.next()));
    double tail = 0.0;
    if (t >= TailStartSeconds) {
        const double u = t - TailStartSeconds;
        tail = TailLevel * m_tailLevel * (1.0 - std::exp(-u / TailAttackSeconds))
          * (TailFast * std::exp(-u / (TailFastSeconds * m_tailScale)) + (1.0 - TailFast) * std::exp(-u / (TailSeconds * m_tailScale)));
    }
    y += tail * m_tailLowPass.process(m_tailBand.process(m_tailNoise.next()));
    if (t > TailStartSeconds + TailAttackSeconds && tail < SilenceLevel && bursts < SilenceLevel) {
        m_active = false;
    }
    return y * ClapGain * m_level;
}

double Clap808Engine::maracasSample(double t)
{
    const double envelope = (1.0 - std::exp(-t / MaracasAttackSeconds))
      * (t < MaracasHoldSeconds ? 1.0 : std::exp(-(t - MaracasHoldSeconds) / MaracasReleaseSeconds));
    if (t > MaracasHoldSeconds && envelope < SilenceLevel) {
        m_active = false;
    }
    return envelope * m_lowPass.process(m_highPass2.process(m_highPass1.process(m_burstNoise.next()))) * MaracasGain;
}

float Clap808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const double t = m_time;
    const double y = m_maracas ? maracasSample(t) : clapSample(t);
    m_time += 1.0 / sampleRate();
    return static_cast<float>(y * m_velocity);
}

bool Clap808Engine::isActive() const
{
    return m_active;
}

void Clap808Engine::reset()
{
    m_active = false;
    for (auto * filter : { &m_burstBand, &m_burstLowPass, &m_tailBand, &m_tailLowPass, &m_highPass1, &m_highPass2, &m_lowPass }) {
        filter->reset();
    }
}

} // namespace noteahead
