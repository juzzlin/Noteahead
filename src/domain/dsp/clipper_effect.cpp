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
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
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

    if (m_mode == Mode::Hard) {
        left = std::clamp(left, -thresholdLin, thresholdLin);
        right = std::clamp(right, -thresholdLin, thresholdLin);
    } else {
        left = thresholdLin * std::tanh(left / thresholdLin);
        right = thresholdLin * std::tanh(right / thresholdLin);
    }

    left *= gainLin;
    right *= gainLin;
}

void ClipperEffect::process(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void ClipperEffect::sync()
{
    syncParameters();
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

} // namespace noteahead
