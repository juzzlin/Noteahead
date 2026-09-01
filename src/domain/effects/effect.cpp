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

Effect::Effect(const Effect & other)
  : DspComponent { other }
  , ParameterContainer { other }
  , m_dryBuffer { other.m_dryBuffer }
  , m_mixLaw { other.m_mixLaw }
  , m_enabled { other.m_enabled }
  , m_bpm { other.m_bpm }
  , m_oversampleFactor { other.m_oversampleFactor }
{
    resolveSharedParameters();
}

Effect & Effect::operator=(const Effect & other)
{
    if (this != &other) {
        DspComponent::operator=(other);
        ParameterContainer::operator=(other);
        m_dryBuffer = other.m_dryBuffer;
        m_mixLaw = other.m_mixLaw;
        m_enabled = other.m_enabled;
        m_bpm = other.m_bpm;
        m_oversampleFactor = other.m_oversampleFactor;
        resolveSharedParameters();
    }
    return *this;
}

Effect::Effect(Effect && other)
  : DspComponent { std::move(other) }
  , ParameterContainer { std::move(other) }
  , m_dryBuffer { std::move(other.m_dryBuffer) }
  , m_mixLaw { other.m_mixLaw }
  , m_enabled { other.m_enabled }
  , m_bpm { other.m_bpm }
  , m_oversampleFactor { other.m_oversampleFactor }
{
    resolveSharedParameters();
    other.resolveSharedParameters();
}

Effect & Effect::operator=(Effect && other)
{
    if (this != &other) {
        DspComponent::operator=(std::move(other));
        ParameterContainer::operator=(std::move(other));
        m_dryBuffer = std::move(other.m_dryBuffer);
        m_mixLaw = other.m_mixLaw;
        m_enabled = other.m_enabled;
        m_bpm = other.m_bpm;
        m_oversampleFactor = other.m_oversampleFactor;
        resolveSharedParameters();
        other.resolveSharedParameters();
    }
    return *this;
}

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
    if (!m_enabled) {
        return;
    }

    // An effect with neither control registered — a filter, an equalizer — has nothing to apply
    // around its own work, so it does not pay for the resolving below.
    if (!m_mixParameter && !m_soloParameter) {
        processSample(left, right);
        return;
    }

    const double dryLeft = left;
    const double dryRight = right;

    processSample(left, right);

    applyBlend(blendState(), dryLeft, dryRight, left, right);
}

void Effect::process(AudioContext & context)
{
    // Disabled means silent, whoever is calling. The racks skip disabled effects before they get
    // here, but the send buses run each effect directly, so the flag has to be honoured at the
    // effect itself or it is honoured only on some paths.
    if (!m_enabled) {
        return;
    }

    setOversampleFactor(context.oversampleFactor);

    const auto blend = blendState();
    if (!blend.solo && !blend.blends) {
        processBlock(context);
        return;
    }

    // Blending and soloing both need the dry signal, and a block-form effect has overwritten it by
    // the time it returns. The copy is only taken when one of them is actually going to be used,
    // into a buffer that is kept between blocks: this runs on the audio thread, which must not
    // allocate.
    const auto sampleCount = static_cast<size_t>(context.frameCount) * 2;
    if (m_dryBuffer.size() < sampleCount) {
        m_dryBuffer.resize(sampleCount);
    }
    std::copy(context.buffer.begin(), context.buffer.begin() + static_cast<ptrdiff_t>(sampleCount), m_dryBuffer.begin());

    processBlock(context);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        applyBlend(blend, m_dryBuffer[i * 2], m_dryBuffer[i * 2 + 1], context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

bool Effect::isSettled() const
{
    return true;
}

void Effect::processBlock(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Effect::resolveSharedParameters()
{
    const auto resolve = [this](const QString & key) -> const Parameter * {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? &parameter->get() : nullptr;
    };
    m_mixParameter = resolve(Constants::NahdXml::xmlKeyMix());
    m_soloParameter = resolve(Constants::NahdXml::xmlKeySolo());
}

void Effect::addMixParameter(float defaultValue, MixLaw law, int xmlMin, int xmlMax, int xmlScale, LegacyNameList legacyNames)
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMix().toStdString(), defaultValue, xmlMin, xmlMax, static_cast<int>(std::lround(static_cast<double>(defaultValue) * xmlScale)), xmlScale, Parameter::Type::Continuous, std::move(legacyNames) });
    m_mixLaw = law;
    resolveSharedParameters();
}

void Effect::setMixLaw(MixLaw law)
{
    m_mixLaw = law;
    // An effect that registers Mix itself has done so by now, so this is where it becomes reachable.
    resolveSharedParameters();
}

float Effect::mix() const
{
    return m_mixParameter ? m_mixParameter->value() : 1.0f;
}

Effect::BlendState Effect::blendState() const
{
    BlendState blend;
    blend.law = m_mixLaw;
    blend.mix = m_mixParameter ? static_cast<double>(m_mixParameter->value()) : 1.0;
    blend.sendMode = m_sendMode;
    // A crossfade at full Mix normally has nothing to do, the wet being the whole output already.
    // On a send bus it does: the dry has to be put back so that the difference the bus takes is the
    // wet rather than the wet minus the source.
    blend.blends = m_mixParameter && m_mixLaw != MixLaw::Internal && (m_sendMode || blend.mix < 1.0 || m_mixLaw != MixLaw::Crossfade);
    // Solo passes only what the effect added, which is what a send bus already does by taking the
    // difference. Honouring it here as well would subtract the dry twice.
    blend.solo = !m_sendMode && m_soloParameter && m_soloParameter->value() > 0.5f;
    return blend;
}

void Effect::applyBlend(const BlendState & blend, double dryLeft, double dryRight, double & left, double & right) const
{
    if (blend.blends && blend.sendMode) {
        // Every law collapses to Additive on a send bus. See setSendMode().
        left = dryLeft + left * blend.mix;
        right = dryRight + right * blend.mix;
    } else if (blend.blends) {
        switch (blend.law) {
        case MixLaw::Crossfade:
            left = dryLeft * (1.0 - blend.mix) + left * blend.mix;
            right = dryRight * (1.0 - blend.mix) + right * blend.mix;
            break;
        case MixLaw::Additive:
            left = dryLeft + left * blend.mix;
            right = dryRight + right * blend.mix;
            break;
        case MixLaw::Internal:
            break;
        case MixLaw::DualSlope: {
            const double dryCoefficient = std::clamp(2.0 * (1.0 - blend.mix), 0.0, 1.0);
            const double wetCoefficient = std::clamp(2.0 * blend.mix, 0.0, 1.0);
            left = dryLeft * dryCoefficient + left * wetCoefficient;
            right = dryRight * dryCoefficient + right * wetCoefficient;
            break;
        }
        }
    }

    if (blend.solo) {
        left -= dryLeft;
        right -= dryRight;
    }
}

void Effect::setSendMode(bool enabled)
{
    m_sendMode = enabled;
}

bool Effect::sendMode() const
{
    return m_sendMode;
}

double Effect::blendWetPart(double dry, double wet, double mix) const
{
    if (m_sendMode) {
        return wet * mix;
    }
    return dry * (1.0 - mix) + wet * mix;
}

double Effect::completeBlend(double dry, double blendedPart, double outputGain) const
{
    if (m_sendMode) {
        return dry + blendedPart * outputGain;
    }
    return blendedPart * outputGain;
}

double Effect::blendWet(double dry, double wet, double mix, double outputGain) const
{
    return completeBlend(dry, blendWetPart(dry, wet, mix), outputGain);
}

void Effect::addSoloParameter()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeySolo().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    resolveSharedParameters();
}

bool Effect::solo() const
{
    return m_soloParameter && m_soloParameter->value() > 0.5f;
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
