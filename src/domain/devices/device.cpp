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

#include "device.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

Device::Device()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyVolume().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -30, 30, 0, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 100, 50 });
}

size_t Device::id() const
{
    return m_id;
}

void Device::setId(size_t id)
{
    m_id = id;
}

void Device::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);
    writer.writeEndElement(); // Device
}

void Device::deserializeFromXml(QXmlStreamReader & reader)
{
    deserializeAttributesFromXml(reader);
    reader.readNext();
    deserializeParametersFromXml(reader);
}

uint32_t Device::sampleRate() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_sampleRate;
}

void Device::setSampleRate(uint32_t sampleRate)
{
    m_sampleRate = sampleRate;
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
        return p->get().value() != oldVal;
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
        return p->get().value() != oldVal;
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
        return p->get().value() != oldVal;
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
        m_linearGain = std::pow(10.0f, ((m_gain - 0.5f) * 60.0f) / 20.0f);
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        m_pan = p->get().value();
    }
}

void Device::reset()
{
    ParameterContainer::reset();
    syncParameters();
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

void Device::serializeAttributesToXml(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::NahdXml::xmlKeyId(), QString::number(m_id));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyCategory(), QString::fromStdString(category()));
    writer.writeAttribute("typeId", QString::fromStdString(typeId()));
}

void Device::deserializeAttributesFromXml(QXmlStreamReader & reader)
{
    if (const auto id = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyId(), false); id.has_value()) {
        m_id = id.value();
    }
}

} // namespace noteahead
