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

#include "reference.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"
#include "../tracker/parameter.hpp"
#include "reference_presets.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Longest reflection any environment asks for, which decides the buffer every one of them gets.
constexpr double MaxTapMs = 250.0;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

//! How hard the drive of an environment that has one pushes at its top setting.
constexpr double MaxDriveGain = 6.0;

const ReferenceEnvironment & environmentAt(size_t index)
{
    const auto & environments = referenceEnvironments();
    return environments.at(std::min(index, environments.size() - 1));
}

} // namespace

void Reference::Voicing::reset()
{
    highPass.reset();
    lowPass.reset();
    for (auto & band : bands) {
        band.reset();
    }
}

double Reference::Voicing::process(double input, size_t bandCount)
{
    double out = highPass.process(input);
    out = lowPass.process(out);
    for (size_t i = 0; i < bandCount && i < bands.size(); i++) {
        out = bands.at(i).process(out);
    }
    return out;
}

void Reference::Reflections::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
    for (auto & filter : damping) {
        filter.reset();
    }
}

double Reference::Reflections::process(double input, const ReferenceEnvironment & environment, double sampleRate, double dampingHz)
{
    if (buffer.empty() || environment.taps.empty()) {
        return 0.0;
    }

    buffer[writePos] = input;

    double sum = 0.0;
    for (size_t i = 0; i < environment.taps.size(); i++) {
        const auto & tap = environment.taps.at(i);
        const auto delaySamples = static_cast<size_t>(tap.delayMs * 0.001 * sampleRate);
        if (!delaySamples || delaySamples >= buffer.size()) {
            continue;
        }
        const auto readPos = (writePos + buffer.size() - delaySamples) % buffer.size();
        // The later a reflection is, the more surfaces and air it has been through, so the darker it
        // comes back: each tap gets its own corner, falling with its delay.
        auto & filter = damping.at(i);
        filter.calculate(dampingHz / (1.0 + static_cast<double>(i) * 0.4), sampleRate);
        filter.process(buffer.at(readPos));
        sum += filter.lowPass() * tap.gain;
    }

    writePos = (writePos + 1) % buffer.size();
    return sum;
}

Reference::Reference()
{
    namespace C = Constants::NahdXml;

    addParameter(Parameter { C::xmlKeyEnvironment().toStdString(), 0.0f, 0, static_cast<int>(referenceEnvironments().size()) - 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyAmount().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyRoom().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyDynamics().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

std::string Reference::typeIdString()
{
    return "a41b6d28-3e95-4c7f-b0a6-2f8d19e35c74";
}

std::string Reference::type() const
{
    return Constants::RackEffectType::reference().toStdString();
}

std::string Reference::typeId() const
{
    return typeIdString();
}

const EffectPresetList & Reference::factoryPresets() const
{
    return ReferencePresets::presets();
}

const ReferenceEnvironment & Reference::environment() const
{
    // Read from the parameter rather than from the cached index: sync() only marks the cache dirty,
    // and the audio thread is what clears it. A caller asking which environment this is must not
    // have to wait for the next block to find out.
    const auto p = parameter(Constants::NahdXml::xmlKeyEnvironment().toStdString());
    return environmentAt(p ? static_cast<size_t>(std::max(0, p->get().xmlValue())) : 0);
}

void Reference::syncParameters()
{
    namespace C = Constants::NahdXml;

    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };
    const auto discrete = [this](const QString & key, int fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().xmlValue() : fallback;
    };

    m_environmentIndex = static_cast<size_t>(std::max(0, discrete(C::xmlKeyEnvironment(), 0)));
    m_amount = value(C::xmlKeyAmount(), 1.0f);
    m_room = value(C::xmlKeyRoom(), 1.0f);
    m_dynamics = value(C::xmlKeyDynamics(), 1.0f);
    m_outputDb = static_cast<float>((value(C::xmlKeyGain(), 0.5f) - 0.5f) * 2.0 * OutputRangeDb);

    m_shouldSyncParameters = false;
}

void Reference::sync()
{
    m_shouldSyncParameters = true;
}

void Reference::updateFilters()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const auto & environment = environmentAt(m_environmentIndex);

    for (auto * voicing : { &m_voicingL, &m_voicingR }) {
        voicing->highPass.calculateLowCut(environment.highPassHz, sampleRate, environment.highPassQ);
        voicing->lowPass.calculateHighCut(std::min(environment.lowPassHz, sampleRate * 0.45), sampleRate, environment.lowPassQ);
        for (size_t i = 0; i < voicing->bands.size(); i++) {
            if (i < environment.bands.size()) {
                const auto & band = environment.bands.at(i);
                voicing->bands.at(i).calculateBell(band.frequencyHz, sampleRate, band.q, band.gainDb);
            } else {
                voicing->bands.at(i).setBypass();
            }
        }
    }

    const auto bufferSize = static_cast<size_t>(MaxTapMs * 0.001 * sampleRate) + 2;
    for (auto * reflections : { &m_reflectionsL, &m_reflectionsR }) {
        reflections->buffer.assign(bufferSize, 0.0);
        reflections->damping.assign(environment.taps.size(), OnePoleFilter {});
        reflections->reset();
    }

    m_compressor.setSampleRate(sampleRate);
    m_compressor.setThresholdDb(environment.thresholdDb);
    m_compressor.setRatio(environment.ratio);
    m_compressor.setKneeDb(6.0);
    m_compressor.setAttackMs(environment.attackMs);
    m_compressor.setReleaseMs(environment.releaseMs);
    m_compressor.reset();
}

void Reference::updateState()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (m_shouldSyncParameters) {
        syncParameters();
    }
    if (std::abs(sampleRate - m_lastSampleRate) < 0.1 && m_environmentIndex == m_lastEnvironmentIndex) {
        return;
    }

    m_lastSampleRate = sampleRate;
    m_lastEnvironmentIndex = m_environmentIndex;
    updateFilters();
}

void Reference::processSample(double & left, double & right)
{
    updateState();

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const auto & environment = environmentAt(m_environmentIndex);
    const double amount = static_cast<double>(m_amount);
    const double dryL = left;
    const double dryR = right;

    // The image first: a phone has one speaker and a laptop's two are a hand's width apart, so what
    // the system keeps of the difference between the channels decides what is left to shape.
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5 * environment.width;
    double wetL = m_voicingL.process(mid + side, environment.bands.size());
    double wetR = m_voicingR.process(mid - side, environment.bands.size());

    // The space it is heard in, if it is heard in one.
    const double room = static_cast<double>(m_room);
    if (room > 0.0 && !environment.taps.empty()) {
        wetL += m_reflectionsL.process(wetL, environment, sampleRate, environment.dampingHz) * room;
        // The right channel reads the same taps from its own buffer, so the two sides do not arrive
        // as one reflection in the middle.
        wetR += m_reflectionsR.process(wetR, environment, sampleRate, environment.dampingHz * 0.85) * room;
    }

    // What the system's own dynamics do to it: the radio's compression, the PA's limiting, and the
    // ceiling a small driver is pushed into.
    const double dynamics = static_cast<double>(m_dynamics);
    if (dynamics > 0.0 && environment.ratio > 1.0) {
        const double gainDb = m_compressor.processGainDb(wetL, wetR) * dynamics;
        const double gain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(gainDb)));
        wetL *= gain;
        wetR *= gain;
    }
    if (dynamics > 0.0 && environment.drive > 0.0) {
        const double drive = 1.0 + environment.drive * dynamics * MaxDriveGain;
        wetL = std::tanh(wetL * drive) / drive;
        wetR = std::tanh(wetR * drive) / drive;
    }

    const double levelLin = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(environment.levelDb)));
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    wetL *= levelLin;
    wetR *= levelLin;

    // Amount is the journey from this room to that one, so that an environment can be leaned into
    // rather than only switched on.
    left = (dryL + (wetL - dryL) * amount) * outputLin;
    right = (dryR + (wetR - dryR) * amount) * outputLin;
}

void Reference::processBlock(AudioContext & context)
{
    if (m_shouldSyncParameters) {
        syncParameters();
    }

    // An export is not listening, and an amount of zero is this room, which needs no work.
    if (context.offline || m_amount <= 0.0f) {
        return;
    }

    Effect::processBlock(context);
}

void Reference::reset()
{
    m_voicingL.reset();
    m_voicingR.reset();
    m_reflectionsL.reset();
    m_reflectionsR.reset();
    m_compressor.reset();
}

} // namespace noteahead
