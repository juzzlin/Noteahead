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
#include "../../common/utils.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <set>

namespace noteahead {

DeviceService::DeviceService(AudioEngineS audioEngine, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
{
    for (int i = 0; i < 128; ++i) {
        m_synthUserPresets[i] = SynthPresets::initPreset();
    }
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

void DeviceService::processMidiProgramChange(const QString & portName, uint8_t program, uint8_t channel)
{
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiProgramChange(program, channel);
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
        if (const auto dev = device(name)) {
            dev->processMidiAllNotesOff();
        }
    }
}

DeviceService::InternalDeviceNames DeviceService::internalDeviceNames() const
{
    return m_audioEngine->deviceNames();
}

QStringList DeviceService::internalDeviceNamesQt() const
{
    QStringList names;
    for (const auto & name : internalDeviceNames()) {
        names << QString::fromStdString(name);
    }
    if (device(Constants::synthDeviceName().toStdString())) {
        names << Constants::synthDeviceName();
    }
    return names;
}

QStringList DeviceService::categories() const
{
    std::set<QString> categories;
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name)) {
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
        if (const auto dev = device(name)) {
            if (QString::fromStdString(dev->category()) == category) {
                devices << QString::fromStdString(name);
            }
        }
    }
    return devices;
}

void DeviceService::setSynthUserPresets(const UserPresets & presets)
{
    m_synthUserPresets = presets;
    for (const auto & name : internalDeviceNames()) {
        if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device(name))) {
            synth->setUserPresets(m_synthUserPresets);
        }
    }
    if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device(Constants::synthDeviceName().toStdString()))) {
        synth->setUserPresets(m_synthUserPresets);
    }
    emit synthUserPresetsChanged(m_synthUserPresets);
}

UserPresets DeviceService::synthUserPresets() const
{
    return m_synthUserPresets;
}

void DeviceService::saveSynthUserPreset(int index, const SynthPreset & preset)
{
    m_synthUserPresets[index] = preset;
    setSynthUserPresets(m_synthUserPresets);
    emit dataChanged();
}

void DeviceService::setProjectPath(const std::string & projectPath)
{
    for (const auto & name : internalDeviceNames()) {
        if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(name))) {
            sampler->setProjectPath(projectPath);
        }
    }
}

void DeviceService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevices());
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name)) {
            dev->serializeToXml(writer);
        }
    }
    if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device(Constants::synthDeviceName().toStdString()))) {
        synth->serializeToXml(writer);
    }

    m_audioEngine->effectRack().serializeToXml(writer);

    if (!m_synthUserPresets.empty()) {
        std::shared_ptr<SynthDevice> synth;
        for (const auto & name : internalDeviceNames()) {
            if (auto dev = std::dynamic_pointer_cast<SynthDevice>(device(name))) {
                synth = dev;
                break;
            }
        }
        const auto typeId = synth ? QString::fromStdString(synth->typeId()) : "";

        writer.writeStartElement(Constants::NahdXml::xmlKeyUserPresets());
        if (!typeId.isEmpty()) {
            writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), typeId);
        }

        for (auto && [index, preset] : m_synthUserPresets) {
            if (!preset.parameters.empty()) {
                writer.writeStartElement(Constants::NahdXml::xmlKeyPreset());
                writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(index));
                writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(preset.name));
                for (auto && [paramName, value] : preset.parameters) {
                    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
                    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(paramName));
                    if (synth) {
                        if (auto p = synth->parameter(paramName); p) {
                            if (p->get().type() == Parameter::Type::Continuous) {
                                writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
                                writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(Parameter::internalToXmlValue(value, p->get().xmlMin(), p->get().xmlMax())));
                                writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), QString::number(p->get().xmlMin()));
                                writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), QString::number(p->get().xmlMax()));
                                writer.writeAttribute(Constants::NahdXml::xmlKeyDefault(), QString::number(p->get().xmlDefault()));
                                writer.writeAttribute(Constants::NahdXml::xmlKeyScale(), QString::number(p->get().xmlScale()));
                            } else if (p->get().type() == Parameter::Type::Discrete) {
                                writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
                                writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(static_cast<int>(std::round(value))));
                            } else if (p->get().type() == Parameter::Type::Boolean) {
                                writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueBool());
                                writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), value > 0.5f ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
                            }
                        } else {
                            writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(static_cast<double>(value)));
                        }
                    } else {
                        writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(static_cast<double>(value)));
                    }
                    writer.writeEndElement(); // Parameter
                }
                writer.writeEndElement(); // Preset
            }
        }
        writer.writeEndElement(); // UserPresets
    }

    writer.writeEndElement(); // Devices
}

void DeviceService::deserializeFromXml(QXmlStreamReader & reader)
{
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyDevice()) {
            const auto category = reader.attributes().value(Constants::NahdXml::xmlKeyCategory()).toString();
            const auto deviceName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            if (const auto dev = device(deviceName)) {
                dev->deserializeFromXml(reader);
            } else {
                reader.skipCurrentElement();
            }
        } else if (reader.name() == Constants::NahdXml::xmlKeySynth()) {
            if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device(Constants::synthDeviceName().toStdString()))) {
                synth->deserializeFromXml(reader);
            }
        } else if (reader.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            m_audioEngine->effectRack().deserializeFromXml(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeyUserPresets()) {
            const auto typeId = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyTypeId(), false);
            
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeyPreset()) {
                    const auto index = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyIndex()).value_or(0);
                    const auto presetName = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName()).value_or("Init");
                    SynthPreset preset { presetName.toStdString(), {} };
                    
                        while (reader.readNextStartElement()) {
                            if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                                const auto paramName = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName()).value_or("").toStdString();
                                if (!paramName.empty()) {
                                    const auto valueType = reader.attributes().value(Constants::NahdXml::xmlKeyParameterValueType()).toString();
                                    const auto xmlValue = reader.attributes().value(Constants::NahdXml::xmlKeyValue()).toString();

                                    if (valueType == Constants::NahdXml::xmlValueInt()) {
                                        preset.parameters[paramName] = static_cast<float>(xmlValue.toInt());
                                    } else if (valueType == Constants::NahdXml::xmlValueBool()) {
                                        preset.parameters[paramName] = (xmlValue == Constants::NahdXml::xmlValueTrue() || xmlValue == "1") ? 1.0f : 0.0f;
                                    } else if (valueType == Constants::NahdXml::xmlValueFloat()) {
                                        const auto xmlMin = reader.attributes().value(Constants::NahdXml::xmlKeyMin()).toInt();
                                        const auto xmlMax = reader.attributes().value(Constants::NahdXml::xmlKeyMax()).toInt();
                                        preset.parameters[paramName] = Parameter::xmlValueToInternal(xmlValue.toInt(), xmlMin, xmlMax);
                                    } else {
                                        // Fallback for older files
                                        const auto xmlMinAttr = reader.attributes().value(Constants::NahdXml::xmlKeyMin());
                                        const auto xmlMaxAttr = reader.attributes().value(Constants::NahdXml::xmlKeyMax());
                                        if (!xmlMinAttr.isNull() && !xmlMaxAttr.isNull()) {
                                            const auto xmlMin = xmlMinAttr.toInt();
                                            const auto xmlMax = xmlMaxAttr.toInt();
                                            const auto intValue = xmlValue.toInt();

                                            std::shared_ptr<SynthDevice> synth;
                                            for (const auto & name : internalDeviceNames()) {
                                                if (auto dev = std::dynamic_pointer_cast<SynthDevice>(device(name))) {
                                                    synth = dev;
                                                    break;
                                                }
                                            }
                                            if (synth) {
                                                if (auto p = synth->parameter(paramName); p) {
                                                    if (p->get().isDiscrete() || p->get().isBoolean()) {
                                                        preset.parameters[paramName] = static_cast<float>(intValue);
                                                    } else {
                                                        preset.parameters[paramName] = Parameter::xmlValueToInternal(intValue, xmlMin, xmlMax);
                                                    }
                                                } else {
                                                    preset.parameters[paramName] = Parameter::xmlValueToInternal(intValue, xmlMin, xmlMax);
                                                }
                                            } else {
                                                preset.parameters[paramName] = Parameter::xmlValueToInternal(intValue, xmlMin, xmlMax);
                                            }
                                        } else {
                                            preset.parameters[paramName] = xmlValue.toFloat();
                                        }
                                    }
                                }
                                reader.skipCurrentElement();
                            } else {
                                reader.skipCurrentElement();
                            }
                        }
                    m_synthUserPresets[index] = std::move(preset);
                } else {
                    reader.skipCurrentElement();
                }
            }
            setSynthUserPresets(m_synthUserPresets);
        } else {
            reader.skipCurrentElement();
        }
    }
    emit dataChanged();
}

EffectRack & DeviceService::effectRack()
{
    return m_audioEngine->effectRack();
}

} // namespace noteahead
