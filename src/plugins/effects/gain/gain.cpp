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

#include "gain.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"
#include "../tracker/parameter.hpp"

namespace noteahead {

namespace {
//! Reach of the trim either way. Matches Wave Designer's output trim, so a gain knob means the same
//! thing wherever it appears in this rack.
constexpr double GainRangeDb = 24.0;
} // namespace

Gain::Gain()
{
    // Centre is unity: a Gain dropped into a rack has to be inaudible until it is moved.
    addParameter({ Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -2400, 2400, 0, 100 });
}

std::string Gain::typeIdString()
{
    return "bfe48a84-3dd9-4403-b62b-73668cb44374";
}

std::string Gain::type() const
{
    return Constants::RackEffectType::gain().toStdString();
}

std::string Gain::typeId() const
{
    return typeIdString();
}

float Gain::gainDb() const
{
    return m_gainDb;
}

ClipDetector & Gain::clipDetector()
{
    return m_clipDetector;
}

const ClipDetector & Gain::clipDetector() const
{
    return m_clipDetector;
}

void Gain::processSample(double & left, double & right)
{
    left *= m_gain;
    right *= m_gain;
}

void Gain::processBlock(AudioContext & context)
{
    Effect::processBlock(context);

    // After the trim, not before: the question the indicator answers is whether this control pushed
    // the signal over, and reading the input could never answer it.
    m_clipDetector.write(context.buffer.data(), context.frameCount);
}

void Gain::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_gainDb = (p->get().value() - 0.5f) * 2.0f * static_cast<float>(GainRangeDb);
        m_gain = static_cast<double>(Utils::Dsp::dbToLinear(m_gainDb));
    }
}

} // namespace noteahead
