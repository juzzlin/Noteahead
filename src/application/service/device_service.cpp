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

#include <set>

namespace noteahead {

DeviceService::DeviceService(AudioEngineS audioEngine, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
{
}

DeviceService::~DeviceService() = default;

void DeviceService::registerDevice(DeviceS device)
{
    connect(device.get(), &Device::dataChanged, this, &DeviceService::dataChanged);
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
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiNoteOn(note, velocity);
    }
}

void DeviceService::processMidiNoteOff(const QString & portName, uint8_t note)
{
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiNoteOff(note);
    }
}

void DeviceService::processMidiCc(const QString & portName, uint8_t controller, uint8_t value, uint8_t channel)
{
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiCc(controller, value, channel);
    }
}

void DeviceService::processMidiAllNotesOff(const QString & portName)
{
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiAllNotesOff();
    }
}

void DeviceService::processMidiAllNotesOff()
{
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name.toStdString()); dev) {
            dev->processMidiAllNotesOff();
        }
    }
}

QStringList DeviceService::internalDeviceNames() const
{
    QStringList names;
    const auto samplerName = Constants::samplerDeviceName();
    if (device(samplerName.toStdString())) {
        names << samplerName;
    }
    return names;
}

QStringList DeviceService::categories() const
{
    std::set<QString> categories;
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name.toStdString()); dev) {
            categories.insert(QString::fromStdString(dev->category()));
        }
    }
    QStringList result;
    for (const auto & c : categories) {
        result << c;
    }
    return result;
}

QStringList DeviceService::devicesByCategory(const QString & category) const
{
    QStringList devices;
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name.toStdString()); dev) {
            if (QString::fromStdString(dev->category()) == category) {
                devices << name;
            }
        }
    }
    return devices;
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
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name.toStdString()); dev) {
            dev->serializeToXml(writer);
        }
    }
    writer.writeEndElement(); // Devices
}

void DeviceService::deserializeFromXml(QXmlStreamReader & reader)
{
    const auto samplerName = Constants::samplerDeviceName().toStdString();
    const auto samplersCategory = Constants::NahdXml::xmlValueSamplers();

    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyDevice()) {
            const auto category = reader.attributes().value(Constants::NahdXml::xmlKeyCategory()).toString();
            const auto deviceName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            if (category == samplersCategory) {
                if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(samplerName)); sampler) {
                    sampler->deserializeFromXml(reader);
                }
            } else if (auto dev = device(deviceName)) {
                dev->deserializeFromXml(reader);
            } else {
                reader.skipCurrentElement();
            }
        } else if (name == Constants::NahdXml::xmlKeySampler()) {
            if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(samplerName)); sampler) {
                sampler->deserializeFromXml(reader);
            } else {
                reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    emit dataChanged();
}

} // namespace noteahead
