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
#include "../../common/constants.hpp"
#include "../dsp/reverb_effect.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

EffectRack::EffectRack()
{
}

EffectRack::~EffectRack() = default;

void EffectRack::addEffect(EffectS effect)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_effects.push_back(std::move(effect));
}

void EffectRack::removeEffect(size_t index)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (index < m_effects.size()) {
        m_effects.erase(m_effects.begin() + index);
    }
}

EffectRack::EffectS EffectRack::effect(size_t index) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (index < m_effects.size()) {
        return m_effects[index];
    }
    return nullptr;
}

size_t EffectRack::effectCount() const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_effects.size();
}

void EffectRack::process(float * output, const float * sendBus, size_t effectIndex, uint32_t nFrames, uint32_t sampleRate)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (effectIndex >= m_effects.size()) return;

    auto & effect = m_effects[effectIndex];
    effect->setSampleRate(sampleRate);

    for (uint32_t i = 0; i < nFrames; ++i) {
        float l = sendBus[i * 2];
        float r = sendBus[i * 2 + 1];
        
        float wetL = l;
        float wetR = r;
        effect->process(wetL, wetR);
        
        // We want only the wet part (assuming Effect::process is dry + wet*mix)
        // wetL is now dry + wet*mix.
        // We subtract the dry part (l, r) to get wet*mix.
        output[i * 2] += (wetL - l);
        output[i * 2 + 1] += (wetR - r);
    }
}

void EffectRack::serializeToXml(QXmlStreamWriter & writer) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    writer.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());
    for (const auto & effect : m_effects) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
        writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(effect->type()));
        effect->serializeParametersToXml(writer);
        writer.writeEndElement(); // Effect
    }
    writer.writeEndElement(); // MasterEffects
}

void EffectRack::deserializeFromXml(QXmlStreamReader & reader)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_effects.clear();

    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
            const auto type = reader.attributes().value(Constants::NahdXml::xmlKeyType()).toString().toStdString();
            EffectS effect;
            if (type == "reverb") {
                effect = std::make_shared<ReverbEffect>();
            }
            
            if (effect) {
                effect->deserializeParametersFromXml(reader);
                effect->sync();
                m_effects.push_back(std::move(effect));
            } else {
                reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }
}

} // namespace noteahead
