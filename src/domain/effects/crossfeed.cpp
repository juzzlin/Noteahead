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

#include "crossfeed.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"
#include "../tracker/parameter.hpp"
#include "crossfeed_presets.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Range of the head delay. The far ear hears a sound around 250-300 us after the near one, which is
//! the time it takes to travel the width of a head; the range either side covers the voicings that
//! deliberately exaggerate or soften that.
constexpr double MinDelayUs = 100.0;
constexpr double MaxDelayUs = 500.0;

//! Corner of the filter the crossfed signal passes through. The head shadows the far ear above a few
//! hundred hertz and hardly at all below it, so the bleed is a low-passed copy rather than a full one.
constexpr double MinCutoffHz = 300.0;
constexpr double MaxCutoffHz = 1500.0;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

//! How much of the side signal is moved to the middle at the top of the Amount control. At 1.0 the
//! bass would be in mono outright, which is a different effect and not the one this is for.
constexpr double MaxAmount = 0.8;

} // namespace

Crossfeed::Crossfeed()
{
    namespace C = Constants::NahdXml;

    addParameter(Parameter { C::xmlKeyAmount().toStdString(), 0.6f, 0, 10000, 6000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyDelay().toStdString(), static_cast<float>((270.0 - MinDelayUs) / (MaxDelayUs - MinDelayUs)), static_cast<int>(MinDelayUs), static_cast<int>(MaxDelayUs), 270, 1, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyCutoff().toStdString(), static_cast<float>(ParameterMapper::unmapLogFrequency(700.0, MinCutoffHz, MaxCutoffHz)), static_cast<int>(MinCutoffHz), static_cast<int>(MaxCutoffHz), 700, 1, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

std::string Crossfeed::typeIdString()
{
    return "5c8f1d47-9a2e-4b06-8f31-e7d4a605b219";
}

std::string Crossfeed::type() const
{
    return Constants::RackEffectType::crossfeed().toStdString();
}

std::string Crossfeed::typeId() const
{
    return typeIdString();
}

const EffectPresetList & Crossfeed::factoryPresets() const
{
    return CrossfeedPresets::presets();
}

void Crossfeed::syncParameters()
{
    namespace C = Constants::NahdXml;

    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    m_amount = value(C::xmlKeyAmount(), 0.6f);
    m_delayUs = static_cast<float>(MinDelayUs + (MaxDelayUs - MinDelayUs) * static_cast<double>(value(C::xmlKeyDelay(), 0.425f)));
    m_cutoffHz = static_cast<float>(ParameterMapper::mapLogFrequency(static_cast<double>(value(C::xmlKeyCutoff(), 0.5f)), MinCutoffHz, MaxCutoffHz));
    m_outputDb = static_cast<float>((value(C::xmlKeyGain(), 0.5f) - 0.5f) * 2.0 * OutputRangeDb);

    m_shouldSyncParameters = false;
}

void Crossfeed::sync()
{
    m_shouldSyncParameters = true;
}

void Crossfeed::updateState()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (m_shouldSyncParameters) {
        syncParameters();
    }
    if (std::abs(sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    m_lastSampleRate = sampleRate;

    // Half a millisecond of head delay is a fraction of a sample at any rate this runs at, so the
    // line is read between taps rather than at one.
    const auto maxDelaySamples = static_cast<size_t>(std::ceil(MaxDelayUs * 1.0e-6 * sampleRate)) + 2;
    m_delay.setMaxDelay(maxDelaySamples);
    m_delay.reset();
}

void Crossfeed::processSample(double & left, double & right)
{
    updateState();

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const double amount = static_cast<double>(m_amount) * MaxAmount;
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));

    m_delay.setFractionalDelay(static_cast<double>(m_delayUs) * 1.0e-6 * sampleRate);
    m_filter.calculate(static_cast<double>(m_cutoffHz), sampleRate);

    // What separates the two channels, which is the only part a far arrival changes. Read before
    // write, so a sample cannot reach the other ear before it has been heard by this one.
    const double side = (left - right) * 0.5;
    const double delayed = m_delay.read();
    m_delay.write(side);
    m_filter.process(delayed);

    // The low end of that difference, late, moved from the near ear to the far one. Taken off one
    // side and added to the other, so nothing in the middle changes level.
    const double bleed = m_filter.lowPass() * amount;
    left = (left - bleed) * outputLin;
    right = (right + bleed) * outputLin;
}

void Crossfeed::processBlock(AudioContext & context)
{
    // Whatever the panel says has to be read before the shortcut below, or a move away from zero
    // would not be heard until something else re-synced the effect.
    if (m_shouldSyncParameters) {
        syncParameters();
    }

    // An export is not listening, and neither is a setting of zero worth walking the buffer for.
    if (context.offline || m_amount <= 0.0f) {
        return;
    }

    Effect::processBlock(context);
}

void Crossfeed::reset()
{
    m_delay.reset();
    m_filter.reset();
}

} // namespace noteahead
