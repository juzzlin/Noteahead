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

#include "simple_eq.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// The fixed "sounds good" contour. Amount scales all three moves from flat to these maxima.
constexpr double BodyFreq = 90.0; // Low-shelf lift for weight and warmth.
constexpr double BodyMaxDb = 6.0;

constexpr double MudFreq = 350.0; // Gentle bell scoop to clear low-mid boxiness.
constexpr double MudMaxDb = 3.0;
constexpr double MudQ = 1.0;

constexpr double AirFreq = 10000.0; // High-shelf lift for openness and sheen.
constexpr double AirMaxDb = 5.0;

constexpr double ShelfQ = 0.707;

// Below this the stage is transparent, so we bypass it to avoid needless coloration and CPU.
constexpr double SilentGainDb = 0.01;

} // namespace

SimpleEq::SimpleEq()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmount().toStdString(), 0.0f, 0, 1000, 0, 1000, Parameter::Type::Continuous });
    // Off, which is also what a project from before the switch existed gets
    addParameter(Parameter { Constants::NahdXml::xmlKeyAutoGain().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    syncParameters();
}

void SimpleEq::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    processStereo(left, right);
}

void SimpleEq::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processStereo(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void SimpleEq::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers) {
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldUpdateBuffers = false;
        m_shouldSyncParameters = true;
    }

    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
}

void SimpleEq::processStereo(double & left, double & right)
{
    left = m_body.left.process(left);
    left = m_mud.left.process(left);
    left = m_air.left.process(left);

    right = m_body.right.process(right);
    right = m_mud.right.process(right);
    right = m_air.right.process(right);

    const double autoGain = m_autoGain.next();
    left *= autoGain;
    right *= autoGain;
}

void SimpleEq::reset()
{
    m_body.reset();
    m_mud.reset();
    m_air.reset();
    m_autoGain.reset();
}

void SimpleEq::sync()
{
    m_shouldUpdateBuffers = true;
}

void SimpleEq::syncParameters()
{
    configureStages(m_body.left, m_mud.left, m_air.left);
    configureStages(m_body.right, m_mud.right, m_air.right);

    // From the stages just configured, which are the ones the audio runs through
    m_autoGain.setTargetDb(autoGainFromParameters() ? autoGainDbFor(m_body.left, m_mud.left, m_air.left) : 0.0, m_sampleRate);
}

void SimpleEq::configureStages(SvfFilter & body, SvfFilter & mud, SvfFilter & air) const
{
    double amount = 0.0;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        amount = static_cast<double>(p->get().value());
    }

    const auto applyStage = [](SvfFilter & filter, auto calc, double gainDb) {
        if (std::abs(gainDb) < SilentGainDb) {
            filter.setBypass();
        } else {
            calc(filter, gainDb);
        }
    };

    const auto bodyShelf = [this](SvfFilter & f, double gainDb) { f.calculateLowShelf(BodyFreq, m_sampleRate, ShelfQ, gainDb); };
    const auto mudBell = [this](SvfFilter & f, double gainDb) { f.calculateBell(MudFreq, m_sampleRate, MudQ, gainDb); };
    const auto airShelf = [this](SvfFilter & f, double gainDb) { f.calculateHighShelf(AirFreq, m_sampleRate, ShelfQ, gainDb); };

    applyStage(body, bodyShelf, amount * BodyMaxDb);
    applyStage(mud, mudBell, -amount * MudMaxDb);
    applyStage(air, airShelf, amount * AirMaxDb);
}

double SimpleEq::autoGainDbFor(const SvfFilter & body, const SvfFilter & mud, const SvfFilter & air) const
{
    AutoGain::Grid response {};
    const auto frequencies = AutoGain::gridFrequencies(m_sampleRate);
    for (size_t i = 0; i < AutoGain::GridSize; i++) {
        const double f = frequencies.at(i);
        const double magnitude = body.magnitudeAt(f, m_sampleRate) * mud.magnitudeAt(f, m_sampleRate) * air.magnitudeAt(f, m_sampleRate);
        response.at(i) = 20.0 * std::log10(magnitude);
    }
    return AutoGain::compensationDb(response);
}

bool SimpleEq::autoGainFromParameters() const
{
    const auto p = parameter(Constants::NahdXml::xmlKeyAutoGain().toStdString());
    return p && p->get().value() >= 0.5f;
}

double SimpleEq::autoGainDb() const
{
    if (!autoGainFromParameters()) {
        return 0.0;
    }
    SvfFilter body;
    SvfFilter mud;
    SvfFilter air;
    configureStages(body, mud, air);
    return autoGainDbFor(body, mud, air);
}

std::string SimpleEq::typeIdString()
{
    return "e8a1c2b3-4d6f-4a7b-8c9d-1e2f3a4b5c6d";
}

std::string SimpleEq::type() const
{
    return Constants::RackEffectType::simpleEq().toStdString();
}

std::string SimpleEq::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
