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
#include <cmath>

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

    applyMix(dryLeft, dryRight, left, right);
    applySolo(dryLeft, dryRight, left, right);
}

void Effect::process(AudioContext & context)
{
    setOversampleFactor(context.oversampleFactor);

    const bool blends = m_mixLaw != MixLaw::Internal && (mix() < 1.0f || m_mixLaw != MixLaw::Crossfade);
    if (!solo() && !blends) {
        processBlock(context);
        return;
    }

    // Blending and soloing both need the dry signal, and a block-form effect has overwritten it by
    // the time it returns. The copy is only taken when one of them is actually going to be used.
    std::vector<double> dry(context.buffer.begin(), context.buffer.begin() + static_cast<ptrdiff_t>(context.frameCount) * 2);

    processBlock(context);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        applyMix(dry[i * 2], dry[i * 2 + 1], context.buffer[i * 2], context.buffer[i * 2 + 1]);
        applySolo(dry[i * 2], dry[i * 2 + 1], context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Effect::processBlock(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Effect::addMixParameter(float defaultValue, MixLaw law, int xmlMin, int xmlMax, int xmlScale, LegacyNameList legacyNames)
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMix().toStdString(), defaultValue, xmlMin, xmlMax, static_cast<int>(std::lround(static_cast<double>(defaultValue) * xmlScale)), xmlScale, Parameter::Type::Continuous, std::move(legacyNames) });
    m_mixLaw = law;
}

void Effect::setMixLaw(MixLaw law)
{
    m_mixLaw = law;
}

float Effect::mix() const
{
    if (const auto parameter = this->parameter(Constants::NahdXml::xmlKeyMix().toStdString()); parameter) {
        return parameter->get().value();
    }
    return 1.0f;
}

void Effect::applyMix(double dryLeft, double dryRight, double & left, double & right) const
{
    const auto parameter = this->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
    if (!parameter) {
        return;
    }

    if (m_mixLaw == MixLaw::Internal) {
        return;
    }

    const double mix = static_cast<double>(parameter->get().value());

    switch (m_mixLaw) {
    case MixLaw::Crossfade:
        left = dryLeft * (1.0 - mix) + left * mix;
        right = dryRight * (1.0 - mix) + right * mix;
        break;
    case MixLaw::Additive:
        left = dryLeft + left * mix;
        right = dryRight + right * mix;
        break;
    case MixLaw::Internal:
        break;
    case MixLaw::DualSlope: {
        const double dryCoefficient = std::clamp(2.0 * (1.0 - mix), 0.0, 1.0);
        const double wetCoefficient = std::clamp(2.0 * mix, 0.0, 1.0);
        left = dryLeft * dryCoefficient + left * wetCoefficient;
        right = dryRight * dryCoefficient + right * wetCoefficient;
        break;
    }
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
