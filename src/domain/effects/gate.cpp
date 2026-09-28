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

#include "gate.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! How far below the open threshold the gate closes, in dB.
//!
//! Fixed rather than exposed: what it is for is stopping a level wobbling about the threshold from
//! rattling the gate, and a user who sets it to zero has only turned the rattle back on. Six dB is
//! the usual figure on hardware gates and is wide enough for programme material.
constexpr double HysteresisDb = 6.0;

//! How fast the peak detector falls, in milliseconds. Short enough to follow a drum's decay and
//! long enough not to fall into the gaps between the cycles of a low note, which would gate a bass
//! on every period.
constexpr double DetectorReleaseMs = 15.0;

//! Level below which the detector is treated as silence, in dB. Stops log(0) and keeps a gate that
//! is fed nothing from reporting a level that wanders with the denormals.
constexpr double SilenceDb = -120.0;

double coefficientFor(double milliseconds, double sampleRate)
{
    if (sampleRate <= 0.0 || milliseconds <= 0.0) {
        return 1.0;
    }
    return 1.0 - std::exp(-1.0 / (milliseconds * 0.001 * sampleRate));
}

} // namespace

Gate::Gate()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyThreshold().toStdString(), 0.333f, -6000, 0, -4000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRatio().toStdString(), 0.0707f, 100, 10000, 800, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRange().toStdString(), 0.6667f, 0, 9000, 6000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.2f, 0, 1000, 200 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHold().toStdString(), 0.1f, 0, 1000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.45f, 0, 1000, 450 });
}

std::string Gate::typeIdString()
{
    return "c1d2e3f4-a5b6-4c7d-8e9f-0a1b2c3d4e5f";
}

std::string Gate::type() const
{
    return Constants::RackEffectType::gate().toStdString();
}

std::string Gate::typeId() const
{
    return typeIdString();
}

double Gate::targetGainDb(double detectorDb) const
{
    // Downward expansion: every dB below the threshold costs (ratio - 1) dB of gain, so a ratio of
    // 1 is no gate at all and a large one is a gate that slams. Floored at the range, which is what
    // lets this tighten a drum rather than only silence it.
    const double below = m_thresholdDb - detectorDb;
    return std::max(-m_rangeDb, -below * (m_ratio - 1.0));
}

void Gate::processSample(double & left, double & right)
{
    // Peak rather than RMS, and instant on the way up: a gate that opened on an average would let
    // the front of every hit through closed.
    const double peak = std::max(std::abs(left), std::abs(right));
    if (peak > m_detector) {
        m_detector = peak;
    } else {
        m_detector += (peak - m_detector) * m_detectorCoefficient;
    }

    const double detectorDb = m_detector > 0.0 ? std::max(SilenceDb, 20.0 * std::log10(m_detector)) : SilenceDb;

    // The hysteresis: open at the threshold, close six dB under it. Consulted in the direction the
    // gate is not currently in, which is what makes it a latch rather than two thresholds.
    if (m_open) {
        m_open = detectorDb > m_thresholdDb - HysteresisDb;
    } else {
        m_open = detectorDb > m_thresholdDb;
    }

    double target {};
    if (m_open) {
        m_holdCounter = static_cast<size_t>(std::max(0.0, m_holdMs * 0.001 * m_sampleRate));
        target = 0.0;
    } else if (m_holdCounter) {
        m_holdCounter--;
        target = 0.0;
    } else {
        target = targetGainDb(detectorDb);
    }

    // Opening is the attack and closing is the release, which is the opposite way round from a
    // compressor: there, attack is the gain coming *down*. A gate's attack is how fast it lets go.
    const double coefficient = target > m_gainDb ? m_attackCoefficient : m_releaseCoefficient;
    m_gainDb += (target - m_gainDb) * coefficient;

    const double gain = std::pow(10.0, m_gainDb / 20.0);
    left *= gain;
    right *= gain;
}

void Gate::processBlock(AudioContext & context)
{
    if (m_sampleRate != context.sampleRate) {
        m_sampleRate = context.sampleRate;
        updateCoefficients();
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Gate::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_thresholdDb = -60.0 + static_cast<double>(p->get().value()) * 60.0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
        m_ratio = static_cast<double>(p->get().xmlValue()) / static_cast<double>(p->get().xmlScale());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRange().toStdString()); p) {
        m_rangeDb = static_cast<double>(p->get().xmlValue()) / static_cast<double>(p->get().xmlScale());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_attackMs = ParameterMapper::mapExponential(p->get().value(), 0.05, 100.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        m_holdMs = static_cast<double>(p->get().value()) * 500.0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_releaseMs = ParameterMapper::mapExponential(p->get().value(), 5.0, 2000.0);
    }
    updateCoefficients();
}

void Gate::reset()
{
    Effect::reset();
    m_detector = 0.0;
    // Open, so that a gate dropped onto a playing track does not swallow the first note while its
    // attack runs. Closing is what it does once it has heard silence, and that costs nothing.
    m_gainDb = 0.0;
    m_holdCounter = 0;
    m_open = false;
}

float Gate::gainDb() const
{
    return static_cast<float>(m_gainDb);
}

void Gate::updateCoefficients()
{
    m_attackCoefficient = coefficientFor(m_attackMs, m_sampleRate);
    m_releaseCoefficient = coefficientFor(m_releaseMs, m_sampleRate);
    m_detectorCoefficient = coefficientFor(DetectorReleaseMs, m_sampleRate);
}

} // namespace noteahead
