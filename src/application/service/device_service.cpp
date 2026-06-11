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

#include "common/constants.hpp"
#include "common/utils.hpp"
#include "domain/devices/device_factory.hpp"
#include "domain/devices/sampler_device.hpp"
#include "domain/devices/synth_device.hpp"
#include "infra/audio/audio_engine.hpp"
#include "infra/data_service.hpp"

#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <set>

namespace noteahead {

DeviceService::DeviceService(AudioEngineS audioEngine, DataServiceS dataService, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
  , m_dataService { std::move(dataService) }
{
    for (int i = 0; i < 128; i++) {
        m_synthUserPresets[i] = SynthPresets::initPreset();
    }
}

DeviceService::~DeviceService() = default;

void DeviceService::setDevice(size_t slotIndex, DeviceS device)
{
    connect(device.get(), &Device::dataChanged, this, &DeviceService::dataChanged);
    device->setId(slotIndex);
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device)) {
        sampler->setProjectPath(m_projectPath);
        sampler->setPathResolver([this](const QString & path) {
            return m_dataService->resolvePath(path);
        });
    }
    m_audioEngine->setDevice(slotIndex, std::move(device));
    emit dataChanged();
}

void DeviceService::clearDevice(size_t slotIndex)
{
    m_audioEngine->clearDevice(slotIndex);
    emit dataChanged();
}

DeviceService::DeviceS DeviceService::device(size_t slotIndex) const
{
    return m_audioEngine->device(slotIndex);
}

DeviceService::DeviceS DeviceService::device(const std::string & name) const
{
    return m_audioEngine->device(name);
}

bool DeviceService::isInternalDevice(const QString & portName) const
{
    return portName.startsWith(Constants::internalDevicePortPrefix());
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

void DeviceService::processMidiPitchBend(const QString & portName, uint16_t value, uint8_t channel)
{
    if (const auto dev = device(portName.toStdString()); dev) {
        dev->processMidiPitchBend(value, channel);
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
    const auto names = internalDeviceNames();
    for (const auto & name : names) {
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
    m_projectPath = projectPath;
    for (size_t i = 0; i < Constants::deviceRackSize(); i++) {
        if (const auto dev = m_audioEngine->device(i)) {
            if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(dev)) {
                sampler->setProjectPath(m_projectPath);
            }
        }
    }
}

std::map<QString, QString> DeviceService::getFilesToEmbed() const
{
    std::map<QString, QString> allFiles;
    for (const auto & name : internalDeviceNames()) {
        if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device(name))) {
            const auto files = sampler->getFilesToEmbed();
            allFiles.insert(files.begin(), files.end());
        }
    }
    return allFiles;
}

void DeviceService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevices());
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name)) {
            dev->serializeToXml(writer);
        }
    }

    writer.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    m_audioEngine->insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeySendEffects());
    m_audioEngine->sendEffectRack().serializeEffectsToXml(writer);

    for (int deviceSlot = 0; deviceSlot < static_cast<int>(Constants::deviceRackSize()); deviceSlot++) {
        if (const auto dev = m_audioEngine->device(deviceSlot)) {
            for (int effectSlot = 0; effectSlot < static_cast<int>(Constants::effectRackSize()); effectSlot++) {
                const float send = dev->reverbSend(effectSlot);
                if (send > 0.0001f) {
                    writer.writeStartElement(Constants::NahdXml::xmlKeySend());
                    writer.writeAttribute(Constants::NahdXml::xmlKeyDeviceSlot(), QString::number(deviceSlot));
                    writer.writeAttribute(Constants::NahdXml::xmlKeyEffectSlot(), QString::number(effectSlot));
                    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(static_cast<double>(send)));
                    writer.writeEndElement(); // Send
                }
            }
        }
    }
    writer.writeEndElement(); // SendEffects
    writer.writeEndElement(); // MasterEffects

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
            const auto deviceName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString();
            const auto typeId = reader.attributes().value(Constants::NahdXml::xmlKeyTypeId()).toString();
            const auto slotAttr = reader.attributes().value(Constants::NahdXml::xmlKeySlot());
            const auto idAttr = reader.attributes().value(Constants::NahdXml::xmlKeyId());

            auto dev = device(deviceName.toStdString());
            if (!dev && !typeId.isEmpty()) {
                dev = DeviceFactory::createDevice(typeId.toStdString(), deviceName.toStdString());

                if (dev) {
                    if (!slotAttr.isNull()) {
                        setDevice(slotAttr.toUInt(), dev);
                    } else if (!idAttr.isNull() && idAttr.toUInt() <= Constants::deviceRackSize()) {
                        setDevice(idAttr.toUInt() - 1, dev);
                    } else {
                        const auto lastSpace = deviceName.lastIndexOf(' ');
                        if (lastSpace != -1) {
                            bool ok;
                            const auto slotIndex = deviceName.mid(lastSpace + 1).toUInt(&ok) - 1;
                            if (ok && slotIndex < Constants::deviceRackSize()) {
                                setDevice(slotIndex, dev);
                            }
                        }
                    }
                    // Re-acquire dev from engine because setDevice moved it
                    dev = device(deviceName.toStdString());
                }
            }

            if (dev) {
                dev->deserializeFromXml(reader);
            } else {
                reader.skipCurrentElement();
            }
        } else if (reader.name() == Constants::NahdXml::xmlKeySynth()) {
            // Handled via generic Device element if present in slot
        } else if (reader.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    m_audioEngine->insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeySendEffects()) {
                    m_audioEngine->sendEffectRack().clear();
                    while (reader.readNextStartElement()) {
                        if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
                            m_audioEngine->sendEffectRack().deserializeEffect(reader);
                        } else if (reader.name() == Constants::NahdXml::xmlKeySend()) {
                            const auto deviceSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyDeviceSlot(), false);
                            const auto effectSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyEffectSlot(), false);
                            const auto value = Utils::Xml::readDoubleAttribute(reader, Constants::NahdXml::xmlKeyValue(), false);
                            if (deviceSlot.has_value() && effectSlot.has_value() && value.has_value()) {
                                if (const auto dev = m_audioEngine->device(static_cast<size_t>(deviceSlot.value()))) {
                                    dev->setReverbSend(static_cast<size_t>(effectSlot.value()), static_cast<float>(value.value()));
                                }
                            }
                            reader.skipCurrentElement();
                        } else {
                            reader.skipCurrentElement();
                        }
                    }
                } else if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
                    // Backward compatibility: effects directly under MasterEffects
                    m_audioEngine->sendEffectRack().deserializeEffect(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeySend()) {
                    // Backward compatibility: sends directly under MasterEffects
                    const auto deviceSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyDeviceSlot(), false);
                    const auto effectSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyEffectSlot(), false);
                    const auto value = Utils::Xml::readDoubleAttribute(reader, Constants::NahdXml::xmlKeyValue(), false);
                    if (deviceSlot.has_value() && effectSlot.has_value() && value.has_value()) {
                        if (const auto dev = m_audioEngine->device(static_cast<size_t>(deviceSlot.value()))) {
                            dev->setReverbSend(static_cast<size_t>(effectSlot.value()), static_cast<float>(value.value()));
                        }
                    }
                    reader.skipCurrentElement();
                } else {
                    reader.skipCurrentElement();
                }
            }
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

void DeviceService::setSamplerAudioFileReaderFactory(SamplerAudioFileReaderFactory factory)
{
    m_samplerAudioFileReaderFactory = std::move(factory);
}

bool DeviceService::exportDeviceSettings(int slotIndex, const QString & filePath) const
{
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QXmlStreamWriter writer { &file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);
    return exportDeviceSettings(slotIndex, writer);
}

bool DeviceService::exportDeviceSettings(int slotIndex, QXmlStreamWriter & writer) const
{
    const auto dev = device(static_cast<size_t>(slotIndex));
    if (!dev) {
        return false;
    }

    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));

    dev->serializeToXml(writer);

    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(dev)) {
        const auto embedFiles = sampler->getFilesToEmbed();
        m_dataService->serializeDataToXml(writer, embedFiles);
    }

    writer.writeEndElement(); // Settings
    writer.writeEndDocument();

    return true;
}

DeviceService::DeviceTypeInfo DeviceService::peekDeviceTypeInfo(const QString & filePath) const
{
    QFile file { filePath };
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QXmlStreamReader reader { &file };
    return peekDeviceTypeInfo(reader);
}

DeviceService::DeviceTypeInfo DeviceService::peekDeviceTypeInfo(QXmlStreamReader & reader) const
{
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                return {
                    reader.attributes().value(Constants::NahdXml::xmlKeyTypeId()).toString(),
                    reader.attributes().value(Constants::NahdXml::xmlKeyTypeName()).toString()
                };
            }
        }
    }
    return {};
}

bool DeviceService::importDeviceSettings(int slotIndex, const QString & filePath)
{
    QFile file { filePath };
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QXmlStreamReader reader { &file };
    return importDeviceSettings(slotIndex, reader);
}

bool DeviceService::importDeviceSettings(int slotIndex, QXmlStreamReader & reader)
{
    while (!reader.atEnd() && !reader.hasError()) {
        const auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeySettings()) {
                while (reader.readNextStartElement()) {
                    if (reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                        const auto typeId = reader.attributes().value(Constants::NahdXml::xmlKeyTypeId()).toString();
                        const auto deviceName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString();

                        auto dev = device(static_cast<size_t>(slotIndex));
                        if (dev && dev->typeId() != typeId.toStdString()) {
                            if (typeId.toStdString() == SamplerDevice::typeIdString() && m_samplerAudioFileReaderFactory) {
                                dev = std::make_shared<SamplerDevice>(deviceName.toStdString(), m_samplerAudioFileReaderFactory());
                            } else {
                                dev = DeviceFactory::createDevice(typeId.toStdString(), deviceName.toStdString());
                            }
                            if (dev) {
                                setDevice(static_cast<size_t>(slotIndex), dev);
                                // Re-acquire dev because setDevice moved it
                                dev = device(static_cast<size_t>(slotIndex));
                            }
                        }

                        if (dev) {
                            dev->deserializeFromXml(reader);
                            dev->setId(static_cast<size_t>(slotIndex));
                        } else {
                            reader.skipCurrentElement();
                        }
                    } else if (reader.name() == Constants::NahdXml::xmlKeyData()) {
                        m_dataService->extractData(reader);
                    } else {
                        reader.skipCurrentElement();
                    }
                }
            }
        }
    }

    if (reader.hasError()) {
        return false;
    }

    emit dataChanged();
    return true;
}

void DeviceService::reset()
{
    m_audioEngine->clear();
    emit dataChanged();
}

EffectRack & DeviceService::sendEffectRack()
{
    return m_audioEngine->sendEffectRack();
}

EffectRack & DeviceService::insertEffectRack()
{
    return m_audioEngine->insertEffectRack();
}

} // namespace noteahead
