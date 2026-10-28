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

#include "noise_source.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Corner of the brown noise integrator. Below it the spectrum is flat instead of running away.
constexpr double BrownCornerHz = 20.0;

//! Kellet's refined pink filter, designed at 44.1 kHz: accurate to ±0.05 dB above 9.2 Hz.
constexpr double KelletSampleRate = 44100.0;
constexpr std::array<double, 6> KelletPoles = { 0.99886, 0.99332, 0.96900, 0.86650, 0.55000, -0.7616 };
constexpr std::array<double, 6> KelletInputGains = { 0.0555179, 0.0750759, 0.1538520, 0.3104856, 0.5329522, -0.0168980 };
constexpr double KelletDirectGain = 0.5362;
constexpr double KelletDelayedGain = 0.115926;

//! Long enough for the slowest pole to decay below double precision at any sample rate.
constexpr double ImpulseResponseSeconds = 1.0;

} // namespace

NoiseSource::NoiseSource(uint32_t seed)
  : m_seed { seed }
  , m_rng { seed }
{
    updateCoefficients();
    updateCrossfade();
}

void NoiseSource::setSampleRate(double sampleRate)
{
    // Devices set the rate on every block, and the normalization below renders a second of
    // impulse response
    if (sampleRate == this->sampleRate()) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updateCoefficients();
    updateCrossfade();
}

void NoiseSource::setColour(double colour)
{
    m_colour = std::clamp(colour, 0.0, 1.0);
    updateCrossfade();
}

double NoiseSource::colour() const
{
    return m_colour;
}

void NoiseSource::setHoldRate(std::optional<double> holdRate)
{
    m_holdRate = holdRate ? std::optional { std::max(0.0, *holdRate) } : std::nullopt;
}

std::optional<double> NoiseSource::holdRate() const
{
    return m_holdRate;
}

void NoiseSource::reset()
{
    m_rng.seed(m_seed);
    m_brownState = 0.0;
    m_pinkStates.fill(0.0);
    m_pinkDelayed = 0.0;
    m_pinkPrevious = 0.0;
    m_holdPhase = 0.0;
    m_heldValue = 0.0;
}

void NoiseSource::updateCoefficients()
{
    m_brownPole = std::exp(-2.0 * std::numbers::pi * BrownCornerHz / m_sampleRate);

    // The positive poles set the corners of the -3 dB/oct staircase, so they are moved to keep their
    // frequencies, and their input gains to keep each section's DC gain g / (1 - p): together that
    // keeps every section's response at the same frequencies. The negative pole only shapes the top
    // octave and stays where it is.
    const double ratio = KelletSampleRate / m_sampleRate;
    for (size_t i = 0; i < m_pinkPoles.size(); i++) {
        const double pole = KelletPoles.at(i);
        const double gain = KelletInputGains.at(i);
        if (pole > 0.0) {
            m_pinkPoles.at(i) = std::pow(pole, ratio);
            m_pinkInputGains.at(i) = gain * (1.0 - m_pinkPoles.at(i)) / (1.0 - pole);
        } else {
            m_pinkPoles.at(i) = pole;
            m_pinkInputGains.at(i) = gain;
        }
    }

    // Energies and cross terms of the unscaled impulse responses. White noise of variance s² through
    // a filter h has variance s² Σh², and two filters of the same noise have covariance s² Σh₁h₂.
    std::array<double, ColourCount> energies {};
    std::array<double, ColourCount - 1> crossTerms {};
    double brown = 0.0;
    auto pinkStates = std::array<double, PinkPoleCount> {};
    double pinkDelayed = 0.0;
    double pinkPrevious = 0.0;
    const int length = static_cast<int>(m_sampleRate * ImpulseResponseSeconds);
    for (int n = 0; n < length; n++) {
        const double input = n == 0 ? 1.0 : 0.0;
        brown = m_brownPole * brown + (1.0 - m_brownPole) * input;
        double pink = KelletDirectGain * input + pinkDelayed;
        pinkDelayed = KelletDelayedGain * input;
        for (size_t i = 0; i < pinkStates.size(); i++) {
            pinkStates.at(i) = m_pinkPoles.at(i) * pinkStates.at(i) + m_pinkInputGains.at(i) * input;
            pink += pinkStates.at(i);
        }
        const double blue = pink - pinkPrevious;
        pinkPrevious = pink;
        const std::array<double, ColourCount> responses = { brown, pink, input, blue };
        for (size_t c = 0; c < energies.size(); c++) {
            energies.at(c) += responses.at(c) * responses.at(c);
        }
        for (size_t c = 0; c < crossTerms.size(); c++) {
            crossTerms.at(c) += responses.at(c) * responses.at(c + 1);
        }
    }

    for (size_t c = 0; c < energies.size(); c++) {
        m_gains.at(c) = 1.0 / std::sqrt(energies.at(c));
    }
    for (size_t c = 0; c < crossTerms.size(); c++) {
        m_correlations.at(c) = crossTerms.at(c) / std::sqrt(energies.at(c) * energies.at(c + 1));
    }
}

void NoiseSource::updateCrossfade()
{
    const double position = m_colour * (ColourCount - 1);
    m_lowerColour = std::min(static_cast<int>(position), ColourCount - 2);
    const double upper = position - m_lowerColour;
    const double lower = 1.0 - upper;

    // Two unit-variance signals with correlation r crossfaded linearly have the variance
    // a² + b² + 2abr, which dips in the middle unless r is 1.
    const double variance = lower * lower + upper * upper + 2.0 * lower * upper * m_correlations.at(m_lowerColour);
    const double compensation = 1.0 / std::sqrt(variance);
    m_lowerGain = lower * compensation * m_gains.at(m_lowerColour);
    m_upperGain = upper * compensation * m_gains.at(m_lowerColour + 1);
}

double NoiseSource::nextColourSample()
{
    const double white = m_dist(m_rng);

    m_brownState = m_brownPole * m_brownState + (1.0 - m_brownPole) * white;

    double pink = KelletDirectGain * white + m_pinkDelayed;
    m_pinkDelayed = KelletDelayedGain * white;
    for (size_t i = 0; i < m_pinkStates.size(); i++) {
        m_pinkStates.at(i) = m_pinkPoles.at(i) * m_pinkStates.at(i) + m_pinkInputGains.at(i) * white;
        pink += m_pinkStates.at(i);
    }
    const double blue = pink - m_pinkPrevious;
    m_pinkPrevious = pink;

    const std::array<double, ColourCount> colours = { m_brownState, pink, white, blue };
    return m_lowerGain * colours.at(m_lowerColour) + m_upperGain * colours.at(m_lowerColour + 1);
}

double NoiseSource::nextSample()
{
    // The filters run on every sample even while held, so releasing the hold does not jump into
    // a stale state.
    const double value = nextColourSample();
    if (!m_holdRate || *m_holdRate >= m_sampleRate) {
        return value;
    }

    m_holdPhase += *m_holdRate / m_sampleRate;
    if (m_holdPhase >= 1.0) {
        m_holdPhase -= std::floor(m_holdPhase);
        m_heldValue = value;
    }
    return m_heldValue;
}

CrackleSource::CrackleSource(uint32_t seed)
  : m_seed { seed }
  , m_rng { seed }
{
}

void CrackleSource::setDensity(double density)
{
    m_density = std::max(0.0, density);
}

double CrackleSource::density() const
{
    return m_density;
}

double CrackleSource::nextSample()
{
    if (m_dist(m_rng) >= m_density / m_sampleRate) {
        return 0.0;
    }

    const double amplitude = m_dist(m_rng);
    const double sign = m_dist(m_rng) < 0.5 ? -1.0 : 1.0;
    return sign * amplitude * amplitude * amplitude;
}

void CrackleSource::reset()
{
    m_rng.seed(m_seed);
}

} // namespace noteahead
