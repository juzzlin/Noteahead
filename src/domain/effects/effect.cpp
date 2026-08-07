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

#include "effect.hpp"

#include "../../common/constants.hpp"

#include <vector>

#include "../dsp/audio_context.hpp"

#include <algorithm>

namespace noteahead {

Effect::~Effect() = default;

Effect::StringList Effect::parameterNames() const
{
    StringList names;
    for (const auto & [name, p] : parameters()) {
        names.push_back(name);
    }
    return names;
}

void Effect::process(double & left, double & right)
{
    const double dryLeft = left;
    const double dryRight = right;

    processSample(left, right);

    applySolo(dryLeft, dryRight, left, right);
}

void Effect::process(AudioContext & context)
{
    setOversampleFactor(context.oversampleFactor);

    if (!solo()) {
        processBlock(context);
        return;
    }

    // Soloing needs the dry signal to subtract, and a block-form effect has overwritten it by the
    // time it returns. The copy is only taken while Solo is engaged, which is a monitoring state.
    std::vector<double> dry(context.buffer.begin(), context.buffer.begin() + static_cast<ptrdiff_t>(context.frameCount) * 2);

    processBlock(context);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        applySolo(dry[i * 2], dry[i * 2 + 1], context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Effect::processBlock(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Effect::addSoloParameter()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeySolo().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
}

bool Effect::solo() const
{
    if (const auto parameter = this->parameter(Constants::NahdXml::xmlKeySolo().toStdString()); parameter) {
        return parameter->get().value() > 0.5f;
    }
    return false;
}

void Effect::applySolo(double dryLeft, double dryRight, double & left, double & right) const
{
    if (solo()) {
        left -= dryLeft;
        right -= dryRight;
    }
}

bool Effect::enabled() const
{
    return m_enabled;
}

void Effect::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void Effect::reset()
{
}

void Effect::sync()
{
}

void Effect::setBpm(float bpm)
{
    m_bpm = std::max(1.0f, bpm);
}

float Effect::bpm() const
{
    return m_bpm;
}

void Effect::setOversampleFactor(uint8_t factor)
{
    m_oversampleFactor = factor;
}

uint8_t Effect::oversampleFactor() const
{
    return m_oversampleFactor;
}

} // namespace noteahead
