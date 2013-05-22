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

#include "vintage_passive_eq.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

// Selectable corner frequencies, mirroring the fixed positions of the classic passive program EQ.
constexpr std::array<double, 4> LowFreqs { 20.0, 30.0, 60.0, 100.0 };
constexpr std::array<double, 7> HighBoostFreqs { 3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0 };
constexpr std::array<double, 3> HighAttenFreqs { 5000.0, 10000.0, 20000.0 };

constexpr double MaxLowBoostDb = 14.0;
constexpr double MaxLowAttenDb = 14.0;
constexpr double MaxHighBoostDb = 18.0;
constexpr double MaxHighAttenDb = 18.0;

// The attenuation shelf sits above the boost shelf: running both produces the trademark bump + dip.
constexpr double LowAttenFreqRatio = 1.5;

// Butterworth Q for the shelving stages; the high-boost bell uses the bandwidth control instead.
constexpr double ShelfQ = 0.707;

// Below this the stage is transparent, so we bypass it to avoid needless coloration and CPU.
constexpr double SilentGainDb = 0.01;

template<typename Table>
double frequencyAt(const Table & table, float value)
{
    const auto index = std::clamp(static_cast<size_t>(std::lround(value)), size_t { 0 }, table.size() - 1);
    return table[index];
}

} // namespace

VintagePassiveEq::VintagePassiveEq()
{
    namespace C = Constants::NahdXml;

    addParameter(Parameter { C::xmlKeyLowFreq().toStdString(), 2.0f, 0, static_cast<int>(LowFreqs.size()) - 1, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyLowBoost().toStdString(), 0.0f, 0, 1400, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyLowAtten().toStdString(), 0.0f, 0, 1400, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyHighBoostFreq().toStdString(), 2.0f, 0, static_cast<int>(HighBoostFreqs.size()) - 1, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyHighBoost().toStdString(), 0.0f, 0, 1800, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyBandwidth().toStdString(), 0.5f, 0, 1000, 500, 1000, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyHighAttenFreq().toStdString(), 1.0f, 0, static_cast<int>(HighAttenFreqs.size()) - 1, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyHighAtten().toStdString(), 0.0f, 0, 1800, 0, 100, Parameter::Type::Continuous });
    // Off, which is also what a project from before the switch existed gets
    addParameter(Parameter { C::xmlKeyAutoGain().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    syncParameters();
}

void VintagePassiveEq::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    processStereo(left, right);
}

void VintagePassiveEq::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processStereo(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void VintagePassiveEq::updateBuffers()
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

void VintagePassiveEq::processStereo(double & left, double & right)
{
    // Passive program EQ is dual mono: each channel runs the same series of stages independently.
    left = m_lowBoost.left.process(left);
    left = m_lowAtten.left.process(left);
    left = m_highBoost.left.process(left);
    left = m_highAtten.left.process(left);

    right = m_lowBoost.right.process(right);
    right = m_lowAtten.right.process(right);
    right = m_highBoost.right.process(right);
    right = m_highAtten.right.process(right);

    const double autoGain = m_autoGain.next();
    left *= autoGain;
    right *= autoGain;
}

void VintagePassiveEq::reset()
{
    m_lowBoost.reset();
    m_lowAtten.reset();
    m_highBoost.reset();
    m_highAtten.reset();
    m_autoGain.reset();
}

void VintagePassiveEq::sync()
{
    m_shouldUpdateBuffers = true;
}

void VintagePassiveEq::syncParameters()
{
    const Stages left { m_lowBoost.left, m_lowAtten.left, m_highBoost.left, m_highAtten.left };
    configureStages(left);
    configureStages({ m_lowBoost.right, m_lowAtten.right, m_highBoost.right, m_highAtten.right });

    // From the stages just configured, which are the ones the audio runs through
    m_autoGain.setTargetDb(autoGainFromParameters() ? autoGainDbFor(left) : 0.0, m_sampleRate);
}

void VintagePassiveEq::configureStages(const Stages & stages) const
{
    namespace C = Constants::NahdXml;

    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? static_cast<double>(p->get().value()) : 0.0;
    };

    const double lowFreq = frequencyAt(LowFreqs, static_cast<float>(valueOf(C::xmlKeyLowFreq())));
    const double lowBoostDb = valueOf(C::xmlKeyLowBoost()) * MaxLowBoostDb;
    const double lowAttenDb = -valueOf(C::xmlKeyLowAtten()) * MaxLowAttenDb;

    const double highBoostFreq = frequencyAt(HighBoostFreqs, static_cast<float>(valueOf(C::xmlKeyHighBoostFreq())));
    const double highBoostDb = valueOf(C::xmlKeyHighBoost()) * MaxHighBoostDb;
    const double bandwidthQ = ParameterMapper::mapExponential(valueOf(C::xmlKeyBandwidth()), 0.3, 3.0);

    const double highAttenFreq = frequencyAt(HighAttenFreqs, static_cast<float>(valueOf(C::xmlKeyHighAttenFreq())));
    const double highAttenDb = -valueOf(C::xmlKeyHighAtten()) * MaxHighAttenDb;

    // Wrap the SvfFilter shelf/bell calculators so the stage helper can call them with a uniform signature.
    const auto lowShelf = [this](SvfFilter & f, double freq, double gainDb, double q) { f.calculateLowShelf(freq, m_sampleRate, q, gainDb); };
    const auto highShelf = [this](SvfFilter & f, double freq, double gainDb, double q) { f.calculateHighShelf(freq, m_sampleRate, q, gainDb); };
    const auto bell = [this](SvfFilter & f, double freq, double gainDb, double q) { f.calculateBell(freq, m_sampleRate, q, gainDb); };

    const auto applyStage = [](SvfFilter & filter, auto calc, double frequency, double q, double gainDb) {
        if (std::abs(gainDb) < SilentGainDb) {
            filter.setBypass();
        } else {
            calc(filter, frequency, gainDb, q);
        }
    };

    applyStage(stages.lowBoost, lowShelf, lowFreq, ShelfQ, lowBoostDb);
    applyStage(stages.lowAtten, lowShelf, lowFreq * LowAttenFreqRatio, ShelfQ, lowAttenDb);
    applyStage(stages.highBoost, bell, highBoostFreq, bandwidthQ, highBoostDb);
    applyStage(stages.highAtten, highShelf, highAttenFreq, ShelfQ, highAttenDb);
}

double VintagePassiveEq::autoGainDbFor(const Stages & stages) const
{
    AutoGain::Grid response {};
    const auto frequencies = AutoGain::gridFrequencies(m_sampleRate);
    for (size_t i = 0; i < AutoGain::GridSize; i++) {
        const double f = frequencies.at(i);
        double magnitude = 1.0;
        for (const SvfFilter * stage : { &stages.lowBoost, &stages.lowAtten, &stages.highBoost, &stages.highAtten }) {
            magnitude *= stage->magnitudeAt(f, m_sampleRate);
        }
        response.at(i) = 20.0 * std::log10(magnitude);
    }
    return AutoGain::compensationDb(response);
}

bool VintagePassiveEq::autoGainFromParameters() const
{
    const auto p = parameter(Constants::NahdXml::xmlKeyAutoGain().toStdString());
    return p && p->get().value() >= 0.5f;
}

double VintagePassiveEq::autoGainDb() const
{
    if (!autoGainFromParameters()) {
        return 0.0;
    }
    SvfFilter lowBoost;
    SvfFilter lowAtten;
    SvfFilter highBoost;
    SvfFilter highAtten;
    const Stages stages { lowBoost, lowAtten, highBoost, highAtten };
    configureStages(stages);
    return autoGainDbFor(stages);
}

std::string VintagePassiveEq::typeIdString()
{
    return "d7c4b1a2-3e5f-4a6b-9c8d-0e1f2a3b4c5d";
}

std::string VintagePassiveEq::type() const
{
    return Constants::RackEffectType::vintagePassiveEq().toStdString();
}

std::string VintagePassiveEq::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
