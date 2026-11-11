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

#include "doubler.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Range of the Time control. Below a couple of milliseconds the copy fuses with the source into a
//! comb that reads as tone rather than as a second part; past fifty it starts to read as an echo.
constexpr double minTimeMs = 2.0;
constexpr double maxTimeMs = 50.0;
constexpr double defaultTimeMs = 15.0;

//! Range the Low Cut sweeps. Far higher than a widener's, because the copy sets each harmonic's
//! level on one side against the other by where that harmonic falls in the comb, and that depends
//! on the note: a note whose fundamental is copied lands wherever its fundamental falls, and some
//! notes are then heard on one side only. Kept above the fundamentals, the copy only reaches the
//! upper harmonics, many to a comb tooth, and they average to the middle for every note.
constexpr double minLowCutHz = 50.0;
constexpr double maxLowCutHz = 5000.0;
constexpr double defaultLowCutHz = 1000.0;
//! A fourth-order Butterworth as two sections: steep enough that the fundamentals just under the
//! corner stay out of the copy too
constexpr double lowCutQ1 = 0.5412;
constexpr double lowCutQ2 = 1.3066;
//! The knob's readout calls the bottom of its travel Bypass up to here.
constexpr float lowCutBypassPosition = 0.0001f;

//! How far either way the delay wanders at full Drift. About what a varispeed tape double moves.
constexpr double maxDriftMs = 1.5;

//! Two slow sines at rates with no simple ratio, so the wander does not repeat audibly
constexpr double driftRateA = 0.37;
constexpr double driftRateB = 0.61;

//! How quickly the delay follows a change of Time. Jumping there would click; gliding is the tape
//! machine's varispeed, which is what is being imitated anyway.
constexpr double timeSmoothingSeconds = 0.05;

//! Room for the longest delay plus its drift, and the interpolating read's extra tap
constexpr double bufferMarginMs = 2.0;

constexpr double silenceThreshold = 1.0e-9;

//! Parameters keep 0..1 in XML as 0..10000
int toXml(double value)
{
    return static_cast<int>(std::lround(value * 10000.0));
}

} // namespace

Doubler::Doubler()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    const double defaultTime = ParameterMapper::unmapExponential(defaultTimeMs, minTimeMs, maxTimeMs);
    const double defaultLowCut = ParameterMapper::unmapLogFrequency(defaultLowCutHz, minLowCutHz, maxLowCutHz);
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayTime().toStdString(), static_cast<float>(defaultTime), 0, 10000, toXml(defaultTime), 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmount().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(defaultLowCut), 0, 10000, toXml(defaultLowCut), 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDepth().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });

    // What this adds is the copy and nothing else, so solo has something exact to isolate
    addSoloParameter();

    syncParameters();
}

std::string Doubler::typeIdString()
{
    return "232cd578-ee42-44b5-9e45-1985e0991e2c";
}

std::string Doubler::type() const
{
    return Constants::RackEffectType::doubler().toStdString();
}

std::string Doubler::typeId() const
{
    return typeIdString();
}

double Doubler::timeMs(double value)
{
    return ParameterMapper::mapExponential(value, minTimeMs, maxTimeMs);
}

void Doubler::prepareBuffer(double sampleRate)
{
    m_delay.setMaxDelay(static_cast<size_t>(std::ceil((maxTimeMs + maxDriftMs + bufferMarginMs) * 0.001 * sampleRate)));
    m_silentFrames = m_delay.capacity();
    m_lowCut.reset();
    m_lowCut2.reset();
}

void Doubler::updateState()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (m_shouldSyncParameters || std::abs(sampleRate - m_lastSampleRate) >= 0.1) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
}

void Doubler::syncParameters()
{
    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const bool rateChanged = std::abs(sampleRate - m_lastSampleRate) >= 0.1;
    if (rateChanged) {
        prepareBuffer(sampleRate);
        m_lastSampleRate = sampleRate;
    }

    const auto mode = value(Constants::NahdXml::xmlKeyMode(), 0.0f) > 0.5f ? Mode::Haas : Mode::MonoSafe;
    if (mode != m_mode) {
        // What is in the line was taken from the other signal: the mid for one mode, the right
        // channel for the other. Played out it would land in the wrong place.
        m_delay.reset();
        m_lowCut.reset();
        m_lowCut2.reset();
        m_mode = mode;
    }

    m_targetDelayFrames = timeMs(value(Constants::NahdXml::xmlKeyDelayTime(), 0.5f)) * 0.001 * sampleRate;
    // Glides only matter while something is in the line to be heard gliding
    if (rateChanged || isSettled()) {
        m_delayFrames = m_targetDelayFrames;
    }
    m_amount = static_cast<double>(value(Constants::NahdXml::xmlKeyAmount(), 0.5f));
    m_driftFrames = static_cast<double>(value(Constants::NahdXml::xmlKeyDepth(), 0.0f)) * maxDriftMs * 0.001 * sampleRate;
    m_smoothing = std::exp(-1.0 / (timeSmoothingSeconds * sampleRate));

    // Fully down the Low Cut is out of the circuit, which is what its readout says there
    const float lowCut = value(Constants::NahdXml::xmlKeyHpfCutoff(), 0.5f);
    const bool lowCutActive = lowCut > lowCutBypassPosition;
    if (lowCutActive && !m_lowCutActive) {
        m_lowCut.reset();
        m_lowCut2.reset();
    }
    m_lowCutActive = lowCutActive;
    const double lowCutHz = ParameterMapper::mapLogFrequency(lowCut, minLowCutHz, maxLowCutHz);
    m_lowCut.calculateLowCut(lowCutHz, sampleRate, lowCutQ1);
    m_lowCut2.calculateLowCut(lowCutHz, sampleRate, lowCutQ2);
}

void Doubler::processSample(double & left, double & right)
{
    updateState();

    m_delayFrames = m_targetDelayFrames + (m_delayFrames - m_targetDelayFrames) * m_smoothing;

    double drift = 0.0;
    if (m_driftFrames > 0.0) {
        constexpr double twoPi = 2.0 * std::numbers::pi;
        drift = (std::sin(twoPi * m_driftPhaseA) + std::sin(twoPi * m_driftPhaseB)) * 0.5 * m_driftFrames;
        m_driftPhaseA = std::fmod(m_driftPhaseA + driftRateA / m_lastSampleRate, 1.0);
        m_driftPhaseB = std::fmod(m_driftPhaseB + driftRateB / m_lastSampleRate, 1.0);
    }
    m_delay.setFractionalDelay(m_delayFrames + drift);

    // Read before write, as DelayLine expects, so the copy is the full delay late
    const double input = m_mode == Mode::Haas ? right : (left + right) * 0.5;
    const double source = m_lowCutActive ? m_lowCut2.process(m_lowCut.process(input)) : input;
    const double copy = m_delay.read();
    m_delay.write(source);

    if (std::abs(source) > silenceThreshold) {
        m_silentFrames = 0;
    } else if (m_silentFrames < m_delay.capacity()) {
        m_silentFrames++;
    }

    if (m_mode == Mode::Haas) {
        // The right channel's upper part is traded for its copy; below the Low Cut it stays put
        right += m_amount * (copy - source);
    } else {
        // Into the side only, with opposite signs on the two channels, so the mono sum loses it
        const double mid = (left + right) * 0.5;
        const double side = (left - right) * 0.5 + m_amount * copy;
        left = mid + side;
        right = mid - side;
    }
}

void Doubler::reset()
{
    m_delay.reset();
    m_lowCut.reset();
    m_lowCut2.reset();
    m_delayFrames = m_targetDelayFrames;
    m_driftPhaseA = 0.0;
    m_driftPhaseB = 0.0;
    m_silentFrames = m_delay.capacity();
}

void Doubler::sync()
{
    m_shouldSyncParameters = true;
}

bool Doubler::isSettled() const
{
    return m_silentFrames >= m_delay.capacity();
}

std::vector<std::string> Doubler::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyAmount().toStdString(),
        NahdXml::xmlKeyDelayTime().toStdString(),
        NahdXml::xmlKeyDepth().toStdString()
    };
}

} // namespace noteahead
