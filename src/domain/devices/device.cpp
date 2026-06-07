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

#include "domain/devices/device.hpp"

#include "common/constants.hpp"
#include "common/parameter_mapper.hpp"
#include "common/utils.hpp"

#include <cmath>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

Device::Device()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyVolume().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -3000, 3000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 10000, 5000, 100 });

    m_reverbSends.resize(Constants::effectRackSize(), 0.0f);
    m_manualReverbSends.resize(Constants::effectRackSize(), 0.0f);
}

size_t Device::id() const
{
    return m_id;
}

void Device::setId(size_t id)
{
    m_id = id;
}

std::vector<MidiCcController> Device::availableMidiCcControllers() const
{
    return {};
}

void Device::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    m_insertEffectRack.serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement(); // Device
}

void Device::deserializeFromXml(QXmlStreamReader & reader)
{
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == QXmlStreamReader::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == QXmlStreamReader::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    m_insertEffectRack.deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

uint32_t Device::sampleRate() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_sampleRate;
}

void Device::setSampleRate(uint32_t sampleRate)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        if (m_sampleRate != sampleRate) {
            m_sampleRate = sampleRate;
            changed = true;
        }
    }
    if (changed) {
        emit sampleRateChanged();
    }
}

std::recursive_mutex & Device::mutex() const
{
    return m_mutex;
}

float Device::volume() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_volume;
}

void Device::setVolume(float volume)
{
    if (updateVolumeParameter(volume, true)) {
        emit dataChanged();
    }
}

float Device::gain() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_gain;
}

void Device::setGain(float gain)
{
    if (updateGainParameter(gain, true)) {
        emit dataChanged();
    }
}

float Device::pan() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_pan;
}

void Device::setPan(float pan)
{
    if (updatePanParameter(pan, true)) {
        emit dataChanged();
    }
}

float Device::reverbSend(size_t index) const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (index < m_reverbSends.size()) {
        return m_reverbSends[index];
    }
    return 0.0f;
}

void Device::setReverbSend(size_t index, float send)
{
    if (updateReverbSendParameter(index, send, true)) {
        emit dataChanged();
    }
}

size_t Device::reverbSendCount() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_reverbSends.size();
}

bool Device::updateVolumeParameter(float volume, bool updateManual)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) {
        const float oldVal = p->get().value();
        p->get().setValue(volume);
        if (updateManual) {
            m_manualVolume = p->get().value();
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updateGainParameter(float gain, bool updateManual)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        const float oldVal = p->get().value();
        p->get().setValue(gain);
        if (updateManual) {
            m_manualGain = p->get().value();
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updatePanParameter(float pan, bool updateManual)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        const float oldVal = p->get().value();
        p->get().setValue(pan);
        if (updateManual) {
            m_manualPan = p->get().value();
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updateReverbSendParameter(size_t index, float send, bool updateManual)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };

    if (index < m_reverbSends.size()) {
        const float oldVal = m_reverbSends[index];
        m_reverbSends[index] = send;
        if (updateManual) {
            if (index < m_manualReverbSends.size()) {
                m_manualReverbSends[index] = send;
            }
        }
        return !qFuzzyCompare(m_reverbSends[index], oldVal);
    }
    return false;
}

void Device::syncParameters()
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) {
        m_volume = p->get().value();
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_gain = p->get().value();
        m_linearGain = static_cast<float>(ParameterMapper::mapDecibel(m_gain, 30.0));
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        m_pan = p->get().value();
    }
}

void Device::setBpm(float bpm)
{
    m_insertEffectRack.setBpm(bpm);
}

void Device::reset()
{
    ParameterContainer::reset();
    syncParameters();
    m_insertEffectRack.reset();
}

void Device::resetAudio()
{
    m_insertEffectRack.reset();
}

void Device::processInsertEffects(AudioContext & context)
{
    m_insertEffectRack.processInPlace(context);
}

EffectRack & Device::insertEffectRack()
{
    return m_insertEffectRack;
}

const EffectRack & Device::insertEffectRack() const
{
    return m_insertEffectRack;
}

float Device::volumeInternal() const
{
    return m_volume;
}

float Device::gainInternal() const
{
    return m_gain;
}

float Device::panInternal() const
{
    return m_pan;
}

float Device::linearGainInternal() const
{
    return m_linearGain;
}

float Device::manualVolumeInternal() const
{
    return m_manualVolume;
}

float Device::manualGainInternal() const
{
    return m_manualGain;
}

float Device::manualPanInternal() const
{
    return m_manualPan;
}

float Device::reverbSendInternal(size_t index) const
{
    if (index < m_reverbSends.size()) {
        return m_reverbSends[index];
    }
    return 0.0f;
}

float Device::manualReverbSendInternal(size_t index) const
{
    if (index < m_manualReverbSends.size()) {
        return m_manualReverbSends[index];
    }
    return 0.0f;
}

void Device::setManualVolume(float volume)
{
    m_manualVolume = volume;
}

void Device::setManualGain(float gain)
{
    m_manualGain = gain;
}

void Device::setManualPan(float pan)
{
    m_manualPan = pan;
}

void Device::setManualReverbSend(size_t index, float send)
{
    if (index < m_manualReverbSends.size()) {
        m_manualReverbSends[index] = send;
    } else {
        m_manualReverbSends.resize(index + 1, 0.0f);
        m_manualReverbSends[index] = send;
    }
}

void Device::serializeAttributesToXml(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::NahdXml::xmlKeySlot(), QString::number(m_id));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeName(), QString::fromStdString(typeName()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyCategory(), QString::fromStdString(category()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(typeId()));
}

void Device::deserializeAttributesFromXml(QXmlStreamReader & reader)
{
    if (const auto slot = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeySlot(), false); slot.has_value()) {
        m_id = slot.value();
    } else if (const auto id = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyId(), false); id.has_value()) {
        m_id = id.value();
    }
}

} // namespace noteahead
