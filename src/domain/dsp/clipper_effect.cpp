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

#include "clipper_effect.hpp"
#include "common/constants.hpp"
#include "common/utils.hpp"
#include "audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

ClipperEffect::ClipperEffect()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyClipperMode().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyClipperThreshold().toStdString(), 1.0f, -2400, 0, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyClipperGain().toStdString(), 0.5f, -2400, 2400, 0, 100 });

    syncParameters();
}

void ClipperEffect::process(double & left, double & right)
{
    const auto thresholdLin = std::max(1e-5, static_cast<double>(Utils::Dsp::dbToLinear(m_thresholdDb)));
    const auto gainLin = static_cast<double>(Utils::Dsp::dbToLinear(m_gainDb));

    const double preL = left;
    const double preR = right;

    if (m_mode == Mode::Hard) {
        left = std::clamp(left, -thresholdLin, thresholdLin);
        right = std::clamp(right, -thresholdLin, thresholdLin);
    } else {
        left = thresholdLin * std::tanh(left / thresholdLin);
        right = thresholdLin * std::tanh(right / thresholdLin);
    }

    const double peakPre = std::max(std::abs(preL), std::abs(preR));
    const double peakPost = std::max(std::abs(left), std::abs(right));

    double reductionDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        reductionDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
    }

    if (reductionDb < m_reductionDb) {
        m_reductionDb = reductionDb;
    } else {
        m_reductionDb = m_meterReleaseCoeff * m_reductionDb + (1.0 - m_meterReleaseCoeff) * reductionDb;
    }

    // Denormal protection
    if (std::abs(m_reductionDb) < 1.0e-15) {
        m_reductionDb = 0.0;
    }

    left *= gainLin;
    right *= gainLin;
}

void ClipperEffect::process(AudioContext & context)
{
    if (static_cast<uint32_t>(context.sampleRate) != m_lastSampleRate) {
        m_meterReleaseCoeff = std::exp(-1.0 / (100.0 * context.sampleRate / 1000.0));
        m_lastSampleRate = static_cast<uint32_t>(context.sampleRate);
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void ClipperEffect::reset()
{
    m_reductionDb = 0.0;
}

void ClipperEffect::sync()
{
    syncParameters();
}

float ClipperEffect::reductionDb() const
{
    return static_cast<float>(m_reductionDb);
}

void ClipperEffect::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyClipperMode().toStdString()); p) {
        m_mode = static_cast<int>(p->get().value()) == 0 ? Mode::Hard : Mode::Soft;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyClipperThreshold().toStdString()); p) {
        m_thresholdDb = -24.0f + p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyClipperGain().toStdString()); p) {
        m_gainDb = -24.0f + p->get().value() * 48.0f;
    }
}

std::string ClipperEffect::typeIdString()
{
    return "9e1f2a3b-4c5d-6e7f-8a9b-0c1d2e3f4a5b";
}

std::string ClipperEffect::type() const
{
    return Constants::RackEffectType::clipper().toStdString();
}

std::string ClipperEffect::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
