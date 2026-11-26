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

#include "true_peak_limiter.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "true_peak_limiter_presets.hpp"

#include <QCoreApplication>

#include <algorithm>

namespace noteahead {

TruePeakLimiter::TruePeakLimiter()
{
    namespace C = Constants::NahdXml;

    addParameter(Parameter { C::xmlKeyThreshold().toStdString(), 1.0f, -2400, 0, 0, 100 });
    addParameter(Parameter { C::xmlKeyCeiling().toStdString(), 5.0f / 6.0f, -600, 0, -100, 100 });
    addParameter(Parameter { C::xmlKeyRelease().toStdString(), 0.667f, 1, 1000, 667 });
    addParameter(Parameter { C::xmlKeyLookahead().toStdString(), 0.5f, 0, 10, 5 });
    addParameter(Parameter { C::xmlKeyBoost().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyAutoRelease().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });

    using Display = ParameterDisplay;
    setParameterDisplay(C::xmlKeyThreshold().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = -24.0, .max = 0.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Threshold") });
    setParameterDisplay(C::xmlKeyCeiling().toStdString(), { .mapping = Display::Mapping::Value, .unit = "dBTP", .min = -6.0, .max = 0.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Ceiling") });
    setParameterDisplay(C::xmlKeyRelease().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 1.0, .max = 1000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Release") });
    setParameterDisplay(C::xmlKeyLookahead().toStdString(), { .mapping = Display::Mapping::Value, .unit = "ms", .min = 0.0, .max = 10.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Lookahead") });

    m_core.setSampleRate(sampleRate());
    m_core.setBpm(static_cast<double>(bpm()));
    syncParameters();
}

std::string TruePeakLimiter::typeIdString()
{
    return "13e8f307-b679-408c-a0f7-f085590442b4";
}

std::string TruePeakLimiter::type() const
{
    return Constants::RackEffectType::truePeakLimiter().toStdString();
}

std::string TruePeakLimiter::typeId() const
{
    return typeIdString();
}

std::vector<std::string> TruePeakLimiter::modulationPriority() const
{
    namespace C = Constants::NahdXml;
    return {
        C::xmlKeyThreshold().toStdString(),
        C::xmlKeyCeiling().toStdString(),
        C::xmlKeyRelease().toStdString(),
    };
}

const EffectPresetList & TruePeakLimiter::factoryPresets() const
{
    return TruePeakLimiterPresets::presets();
}

void TruePeakLimiter::setSampleRate(double sampleRate)
{
    Effect::setSampleRate(sampleRate);
    m_core.setSampleRate(sampleRate);
}

void TruePeakLimiter::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    m_core.setBpm(static_cast<double>(bpm));
}

size_t TruePeakLimiter::latencyFrames(double sampleRate) const
{
    // Read from parameter rather than cached core state so offline renders get exact latency before processing.
    const auto p { parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()) };
    const double lookaheadMs { p ? static_cast<double>(p->get().value()) * 10.0 : 5.0 };
    return TruePeakLimiterCore::latencyFramesFor(sampleRate, lookaheadMs);
}

bool TruePeakLimiter::isSettled() const
{
    return m_core.isSettled();
}

void TruePeakLimiter::processSample(double & left, double & right)
{
    syncParameters();
    m_core.processSample(left, right);
}

void TruePeakLimiter::processBlock(AudioContext & context)
{
    syncParameters();
    m_core.processBlock(std::span<double> { context.buffer.data(), static_cast<size_t>(context.frameCount) * 2 }, context.frameCount);
}

void TruePeakLimiter::reset()
{
    m_core.reset();
}

void TruePeakLimiter::sync()
{
    syncParameters();
}

float TruePeakLimiter::reductionDb() const
{
    return m_core.reductionDb();
}

float TruePeakLimiter::outputTruePeakDb() const
{
    return m_core.outputTruePeakDb();
}

float TruePeakLimiter::outputTruePeakHoldDb() const
{
    return m_core.outputTruePeakHoldDb();
}

void TruePeakLimiter::resetPeakHold()
{
    m_core.resetPeakHold();
}

float TruePeakLimiter::thresholdPosition(double db)
{
    return static_cast<float>(std::clamp((db + 24.0) / 24.0, 0.0, 1.0));
}

float TruePeakLimiter::ceilingPosition(double db)
{
    return static_cast<float>(std::clamp((db + 6.0) / 6.0, 0.0, 1.0));
}

float TruePeakLimiter::releasePosition(double ms)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(ms, 1.0, 1000.0), 0.0, 1.0));
}

float TruePeakLimiter::lookaheadPosition(double ms)
{
    return static_cast<float>(std::clamp(ms / 10.0, 0.0, 1.0));
}

void TruePeakLimiter::syncParameters()
{
    namespace C = Constants::NahdXml;

    if (const auto p { parameter(C::xmlKeyThreshold().toStdString()) }; p) {
        m_core.setThresholdDb(-24.0 + static_cast<double>(p->get().value()) * 24.0);
    }
    if (const auto p { parameter(C::xmlKeyCeiling().toStdString()) }; p) {
        m_core.setCeilingDb(-6.0 + static_cast<double>(p->get().value()) * 6.0);
    }
    if (const auto p { parameter(C::xmlKeyRelease().toStdString()) }; p) {
        m_core.setReleaseMs(ParameterMapper::mapExponential(static_cast<double>(p->get().value()), 1.0, 1000.0));
    }
    if (const auto p { parameter(C::xmlKeyLookahead().toStdString()) }; p) {
        m_core.setLookaheadMs(static_cast<double>(p->get().value()) * 10.0);
    }
    if (const auto p { parameter(C::xmlKeyBoost().toStdString()) }; p) {
        m_core.setBoost(p->get().value() > 0.5f);
    }
    if (const auto p { parameter(C::xmlKeyAutoRelease().toStdString()) }; p) {
        m_core.setAutoRelease(p->get().value() > 0.5f);
    }
}

} // namespace noteahead
