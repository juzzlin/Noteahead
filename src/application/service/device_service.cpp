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

#include "device_service.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

DeviceService::DeviceService(AudioEngineS audioEngine, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
{
}

DeviceService::~DeviceService() = default;

void DeviceService::registerDevice(DeviceS device)
{
    m_audioEngine->addDevice(std::move(device));
}

void DeviceService::unregisterDevice(const std::string & name)
{
    m_audioEngine->removeDevice(name);
}

DeviceService::DeviceS DeviceService::device(const std::string & name) const
{
    return m_audioEngine->device(name);
}

bool DeviceService::isInternalDevice(const QString & portName) const
{
    return m_audioEngine->device(portName.toStdString()) != nullptr;
}

void DeviceService::processMidiNoteOn(const QString & portName, uint8_t note, uint8_t velocity)
{
    if (auto dev = device(portName.toStdString()); dev) {
        dev->processMidiNoteOn(note, velocity);
    }
}

void DeviceService::processMidiNoteOff(const QString & portName, uint8_t note)
{
    if (auto dev = device(portName.toStdString()); dev) {
        dev->processMidiNoteOff(note);
    }
}

void DeviceService::processMidiCc(const QString & portName, uint8_t controller, uint8_t value)
{
    if (auto dev = device(portName.toStdString()); dev) {
        dev->processMidiCc(controller, value);
    }
}

void DeviceService::processMidiAllNotesOff(const QString & portName)
{
    if (auto dev = device(portName.toStdString()); dev) {
        dev->processMidiAllNotesOff();
    }
}

QStringList DeviceService::internalDeviceNames() const
{
    QStringList names;
    if (device(Constants::samplerDeviceName().toStdString())) {
        names << Constants::samplerDeviceName();
    }
    return names;
}

void DeviceService::setProjectPath(const std::string & projectPath)
{
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(Constants::samplerDeviceName().toStdString())); sampler) {
        sampler->setProjectPath(projectPath);
    }
}

void DeviceService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevices());
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(Constants::samplerDeviceName().toStdString())); sampler) {
        sampler->serializeToXml(writer);
    }
    writer.writeEndElement(); // Devices
}

void DeviceService::deserializeFromXml(QXmlStreamReader & reader)
{
    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == Constants::NahdXml::xmlKeyDevices())) {
        if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeySampler()) {
            if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(Constants::samplerDeviceName().toStdString())); sampler) {
                sampler->deserializeFromXml(reader);
            }
        }
        reader.readNext();
    }
}

} // namespace noteahead
