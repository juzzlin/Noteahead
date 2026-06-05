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

#include "effect_rack.hpp"

#include "effect_factory.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <QXmlStreamReader>

namespace noteahead {

EffectRack::EffectRack()
{
    m_effects.resize(Constants::effectRackSize(), nullptr);
}

EffectRack::~EffectRack() = default;

void EffectRack::setEffect(size_t index, EffectS effect)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (index < m_effects.size()) {
        m_effects[index] = std::move(effect);
    }
}

void EffectRack::removeEffect(size_t index)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (index < m_effects.size()) {
        m_effects.erase(m_effects.begin() + index);
    }
}

EffectRack::EffectS EffectRack::effect(size_t index) const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (index < m_effects.size()) {
        return m_effects[index];
    }
    return nullptr;
}

std::vector<EffectRack::EffectS> EffectRack::effects() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_effects;
}

size_t EffectRack::effectCount() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_effects.size();
}

void EffectRack::process(AudioContext & outputContext, const double * sendBus, size_t effectIndex)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (effectIndex >= m_effects.size())
        return;

    auto & effect = m_effects[effectIndex];
    if (!effect || !effect->enabled())
        return;

    effect->setSampleRate(outputContext.sampleRate);
    effect->sync();

    // Call block-based process if available (via default implementation or override)
    // but we need to mix into outputContext.
    // The previous implementation was doing a sample-by-sample delta mix.

    for (uint32_t i = 0; i < outputContext.frameCount; i++) {
        double l = sendBus[i * 2];
        double r = sendBus[i * 2 + 1];

        double wetL = l;
        double wetR = r;
        effect->process(wetL, wetR);

        outputContext.buffer[i * 2] += (wetL - l);
        outputContext.buffer[i * 2 + 1] += (wetR - r);
    }
}

void EffectRack::processInPlace(AudioContext & context)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    for (auto & effect : m_effects) {
        if (!effect || !effect->enabled())
            continue;

        effect->setSampleRate(context.sampleRate);
        effect->process(context);
    }
}

void EffectRack::reset()
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    for (auto & effect : m_effects) {
        if (effect) {
            effect->reset();
        }
    }
}

void EffectRack::setBpm(float bpm)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    for (auto & effect : m_effects) {
        if (effect) {
            effect->setBpm(bpm);
        }
    }
}

void EffectRack::clear()
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    std::fill(m_effects.begin(), m_effects.end(), nullptr);
}

void EffectRack::serializeEffectsToXml(QXmlStreamWriter & writer) const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    for (size_t i = 0; i < m_effects.size(); i++) {
        const auto & effect = m_effects[i];
        if (effect) {
            writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
            writer.writeAttribute("slot", QString::number(i));
            writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(effect->typeId()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(effect->type()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), effect->enabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
            effect->serializeParametersToXml(writer);
            writer.writeEndElement(); // Effect
        }
    }
}

void EffectRack::deserializeEffectsFromXml(QXmlStreamReader & reader)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    std::fill(m_effects.begin(), m_effects.end(), nullptr);

    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
            deserializeEffect(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
}

void EffectRack::deserializeEffect(QXmlStreamReader & reader)
{
    const auto slot = Utils::Xml::readIntAttribute(reader, "slot", false);
    const auto typeId = reader.attributes().value(Constants::NahdXml::xmlKeyTypeId()).toString().toStdString();
    const auto type = reader.attributes().value(Constants::NahdXml::xmlKeyType()).toString().toStdString();
    const auto enabled = reader.attributes().value(Constants::NahdXml::xmlKeyEnabled()).toString() != Constants::NahdXml::xmlValueFalse();

    const auto effect = EffectFactory::createEffect(typeId, type);

    if (effect) {
        effect->setEnabled(enabled);
        effect->deserializeParametersFromXml(reader);
        effect->sync();
        const size_t targetIndex = slot.has_value() ? static_cast<size_t>(slot.value()) : 0;
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        if (targetIndex >= m_effects.size()) {
            m_effects.resize(targetIndex + 1, nullptr);
        }
        m_effects[targetIndex] = std::move(effect);
    } else {
        reader.skipCurrentElement();
    }
}

} // namespace noteahead
