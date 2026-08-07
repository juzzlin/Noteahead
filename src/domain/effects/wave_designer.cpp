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

#include "wave_designer.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Time constants of the three followers, in milliseconds.
//!
//! Attack works on the gap between the fast and the medium one, which opens only while a hit is
//! rising. Sustain works on the gap between the medium and the slow one, which stays open through
//! the decay. The medium follower belongs to both, so a hit shaped at one end is not shaped twice at
//! the other.
constexpr double FastAttackMs = 0.05;
constexpr double FastReleaseMs = 30.0;
constexpr double MediumAttackMs = 12.0;
constexpr double MediumReleaseMs = 150.0;
constexpr double SlowAttackMs = 12.0;
constexpr double SlowReleaseMs = 900.0;

//! How much gain each control reaches at its extremes, in dB. Symmetric, so the same control tames
//! a transient as far as it lifts one.
constexpr double MaxShapingDb = 15.0;

//! The follower difference that counts as a full-scale transient. Differences are in dB, and a hit
//! whose fast follower is this far above its medium one is as sharp an edge as the control needs to
//! see to give all it has.
constexpr double FullAttackDb = 12.0;
constexpr double FullSustainDb = 9.0;

//! Output trim range either side of unity, in dB, as the X32's Wave Designer has it.
constexpr double GainRangeDb = 24.0;

//! Smoothing applied to the rectified signal before the followers see it.
//!
//! Without it the followers are fed the ripple of the waveform itself, and since they smooth that
//! ripple by different amounts they settle at different levels even on a held tone. That difference
//! reads as sustain, so a steady note would be shaped by a control that is only supposed to act on
//! what changes. Short enough to leave the edge of a hit intact.
constexpr double DetectorSmoothingMs = 2.0;

//! Response of the meter, which follows the gain being applied rather than the audio.
constexpr double MeterReleaseMs = 120.0;

double coefficientFor(double milliseconds, double sampleRate)
{
    return std::exp(-1.0 / (std::max(milliseconds, 0.001) * sampleRate / 1000.0));
}

} // namespace

double WaveDesigner::Follower::process(double rectified)
{
    const double coefficient = rectified > level ? attackCoefficient : releaseCoefficient;
    level = coefficient * level + (1.0 - coefficient) * rectified;
    return level;
}

void WaveDesigner::Follower::reset()
{
    level = 0.0;
}

WaveDesigner::WaveDesigner()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.5f, -10000, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeySustain().toStdString(), 0.5f, -10000, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -2400, 2400, 0, 100, Parameter::Type::Continuous });
    addMixParameter(1.0f);

    syncParameters();
}

std::string WaveDesigner::typeIdString()
{
    return "b71f4c58-2e09-4a6d-9c83-7d514b0e6a92";
}

std::string WaveDesigner::type() const
{
    return Constants::RackEffectType::waveDesigner().toStdString();
}

std::string WaveDesigner::typeId() const
{
    return typeIdString();
}

void WaveDesigner::updateCoefficients()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (std::abs(sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }
    m_lastSampleRate = sampleRate;

    m_detectorCoefficient = coefficientFor(DetectorSmoothingMs, sampleRate);
    m_fast.attackCoefficient = coefficientFor(FastAttackMs, sampleRate);
    m_fast.releaseCoefficient = coefficientFor(FastReleaseMs, sampleRate);
    m_medium.attackCoefficient = coefficientFor(MediumAttackMs, sampleRate);
    m_medium.releaseCoefficient = coefficientFor(MediumReleaseMs, sampleRate);
    m_slow.attackCoefficient = coefficientFor(SlowAttackMs, sampleRate);
    m_slow.releaseCoefficient = coefficientFor(SlowReleaseMs, sampleRate);
}

void WaveDesigner::processSample(double & left, double & right)
{
    updateCoefficients();

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;

    // One detector for the pair, so a hit on either side shapes both and the image cannot shift.
    const double rectified = std::max(std::abs(left), std::abs(right));
    m_detector = m_detectorCoefficient * m_detector + (1.0 - m_detectorCoefficient) * rectified;

    const double fast = m_fast.process(m_detector);
    const double medium = m_medium.process(m_detector);
    const double slow = m_slow.process(m_detector);

    // The differences are taken in dB, which is what makes the shaper level independent: halving
    // the input halves every follower and leaves the ratios between them where they were.
    constexpr double floorLevel = 1.0e-7;
    const double fastDb = Utils::Dsp::linearToDb(static_cast<float>(std::max(fast, floorLevel)));
    const double mediumDb = Utils::Dsp::linearToDb(static_cast<float>(std::max(medium, floorLevel)));
    const double slowDb = Utils::Dsp::linearToDb(static_cast<float>(std::max(slow, floorLevel)));

    // The fast follower runs above the medium one while a hit rises, and the slow one stays above
    // the medium one while it falls: the two differences are therefore taken in opposite directions.
    const double attackAmount = std::clamp((fastDb - mediumDb) / FullAttackDb, 0.0, 1.0);
    const double sustainAmount = std::clamp((slowDb - mediumDb) / FullSustainDb, 0.0, 1.0);

    const double shapingDb = static_cast<double>(m_attack) * MaxShapingDb * attackAmount
      + static_cast<double>(m_sustain) * MaxShapingDb * sustainAmount;

    const double gain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(shapingDb + static_cast<double>(m_gainDb))));

    left *= gain;
    right *= gain;

    const double meterReleaseCoefficient = std::exp(-1.0 / (MeterReleaseMs * sampleRate / 1000.0));
    if (std::abs(shapingDb) > std::abs(m_shapingDb)) {
        m_shapingDb = shapingDb;
    } else {
        m_shapingDb = meterReleaseCoefficient * m_shapingDb + (1.0 - meterReleaseCoefficient) * shapingDb;
    }

    // Denormal protection
    if (std::abs(m_shapingDb) < 1.0e-15) {
        m_shapingDb = 0.0;
    }
}

float WaveDesigner::shapingDb() const
{
    return static_cast<float>(m_shapingDb);
}

void WaveDesigner::reset()
{
    m_detector = 0.0;
    m_fast.reset();
    m_medium.reset();
    m_slow.reset();
    m_shapingDb = 0.0;
}

void WaveDesigner::syncParameters()
{
    // Attack and Sustain are bipolar: the parameter runs 0..1 with the centre as no shaping.
    if (const auto parameter = this->parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); parameter) {
        m_attack = (parameter->get().value() - 0.5f) * 2.0f;
    }
    if (const auto parameter = this->parameter(Constants::NahdXml::xmlKeySustain().toStdString()); parameter) {
        m_sustain = (parameter->get().value() - 0.5f) * 2.0f;
    }
    if (const auto parameter = this->parameter(Constants::NahdXml::xmlKeyGain().toStdString()); parameter) {
        m_gainDb = (parameter->get().value() - 0.5f) * 2.0f * static_cast<float>(GainRangeDb);
    }
}

void WaveDesigner::sync()
{
    syncParameters();
}

} // namespace noteahead
