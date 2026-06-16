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
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QDateTime>
#include <QFile>
#include <QVariant>

#include <format>
#include <set>

namespace noteahead {

static const auto TAG = "DeviceService";

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

std::shared_ptr<SynthDevice> DeviceService::findFirstSynthDevice() const
{
    for (const auto & name : internalDeviceNames()) {
        if (auto dev = std::dynamic_pointer_cast<SynthDevice>(device(name))) {
            return dev;
        }
    }
    return {};
}

void DeviceService::serializeDevices(ProjectWriter & writer) const
{
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name)) {
            dev->serializeToXml(writer);
        }
    }
}

void DeviceService::serializeReverbSends(ProjectWriter & writer) const
{
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
}

void DeviceService::serializeSendEffects(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeySendEffects());
    m_audioEngine->sendEffectRack().serializeEffectsToXml(writer);
    serializeReverbSends(writer);
    writer.writeEndElement(); // SendEffects
}

void DeviceService::serializeMasterEffects(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    m_audioEngine->insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement(); // InsertEffects

    serializeSendEffects(writer);

    writer.writeEndElement(); // MasterEffects
}

void DeviceService::serializePresetParameter(ProjectWriter & writer, const std::string & paramName, float value, const std::shared_ptr<SynthDevice> & synth) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(paramName));

    const auto p = synth ? synth->parameter(paramName) : std::nullopt;
    if (!p) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(static_cast<double>(value)));
    } else if (p->get().type() == Parameter::Type::Continuous) {
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

    writer.writeEndElement(); // Parameter
}

void DeviceService::serializePreset(ProjectWriter & writer, int index, const SynthPreset & preset, const std::shared_ptr<SynthDevice> & synth) const
{
    if (preset.parameters.empty()) {
        return;
    }

    writer.writeStartElement(Constants::NahdXml::xmlKeyPreset());
    writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(index));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(preset.name));
    for (auto && [paramName, value] : preset.parameters) {
        serializePresetParameter(writer, paramName, value, synth);
    }
    writer.writeEndElement(); // Preset
}

void DeviceService::serializeUserPresets(ProjectWriter & writer) const
{
    if (m_synthUserPresets.empty()) {
        return;
    }

    const auto synth = findFirstSynthDevice();
    const auto typeId = synth ? QString::fromStdString(synth->typeId()) : "";

    writer.writeStartElement(Constants::NahdXml::xmlKeyUserPresets());
    if (!typeId.isEmpty()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), typeId);
    }

    for (auto && [index, preset] : m_synthUserPresets) {
        serializePreset(writer, index, preset, synth);
    }

    writer.writeEndElement(); // UserPresets
}

void DeviceService::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevices());

    serializeDevices(writer);
    serializeMasterEffects(writer);
    serializeUserPresets(writer);

    writer.writeEndElement(); // Devices
}

DeviceService::DeviceS DeviceService::getDevice(std::string name, std::string typeId)
{
    if (const auto dev = device(name); !dev && !typeId.empty()) {
        return DeviceFactory::createDevice(typeId, name);
    } else {
        return dev;
    }
}

void DeviceService::deserializeDevice(ProjectReader & reader)
{
    const auto name = reader.attribute(Constants::NahdXml::xmlKeyName()).toString();
    const auto typeId = reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString();
    const auto slotAttr = reader.attribute(Constants::NahdXml::xmlKeySlot());
    if (slotAttr.isNull() || slotAttr.toUInt() > Constants::deviceRackSize()) {
        juzzlin::L(TAG).warning() << std::format("Skipping device {} ({}) with slot index {} out of bounds!", typeId.toStdString(), name.toStdString(), slotAttr.toUInt());
        reader.skipCurrentElement();
        return;
    }
    if (const auto dev = getDevice(name.toStdString(), typeId.toStdString()); dev) {
        setDevice(slotAttr.toUInt(), dev);
        dev->deserializeFromXml(reader);
    } else {
        juzzlin::L(TAG).error() << std::format("Failed to create device {} ({}) with slot index {}", typeId.toStdString(), name.toStdString(), slotAttr.toUInt());
        reader.skipCurrentElement();
    }
}

void DeviceService::deserializeEffectSend(ProjectReader & reader)
{
    const auto deviceSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyDeviceSlot(), false);
    const auto effectSlot = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyEffectSlot(), false);
    const auto value = Utils::Xml::readDoubleAttribute(reader, Constants::NahdXml::xmlKeyValue(), false);
    if (deviceSlot.has_value() && effectSlot.has_value() && value.has_value()) {
        if (const auto dev = m_audioEngine->device(static_cast<size_t>(deviceSlot.value()))) {
            dev->setReverbSend(static_cast<size_t>(effectSlot.value()), static_cast<float>(value.value()));
        }
    }
    reader.skipCurrentElement();
}

void DeviceService::deserializeSendEffects(ProjectReader & reader)
{
    m_audioEngine->sendEffectRack().clear();
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
            m_audioEngine->sendEffectRack().deserializeEffect(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeySend()) {
            deserializeEffectSend(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
}

void DeviceService::deserializeMasterEffects(ProjectReader & reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
            m_audioEngine->insertEffectRack().deserializeEffectsFromXml(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeySendEffects()) {
            deserializeSendEffects(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
            // Backward compatibility: effects directly under MasterEffects
            m_audioEngine->sendEffectRack().deserializeEffect(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeySend()) {
            // Backward compatibility: sends directly under MasterEffects
            deserializeEffectSend(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
}

float DeviceService::legacyPresetParameterValue(ProjectReader & reader, const std::string & paramName, const QString & xmlValue) const
{
    const auto xmlMinAttr = reader.attribute(Constants::NahdXml::xmlKeyMin());
    const auto xmlMaxAttr = reader.attribute(Constants::NahdXml::xmlKeyMax());
    if (xmlMinAttr.isNull() || xmlMaxAttr.isNull()) {
        return xmlValue.toFloat();
    }

    const auto xmlMin = xmlMinAttr.toInt();
    const auto xmlMax = xmlMaxAttr.toInt();
    const auto intValue = xmlValue.toInt();

    const auto synth = findFirstSynthDevice();
    if (!synth) {
        return Parameter::xmlValueToInternal(intValue, xmlMin, xmlMax);
    }

    if (const auto p = synth->parameter(paramName); p && (p->get().isDiscrete() || p->get().isBoolean())) {
        return static_cast<float>(intValue);
    }
    return Parameter::xmlValueToInternal(intValue, xmlMin, xmlMax);
}

void DeviceService::deserializePresetParameter(ProjectReader & reader, SynthPreset & preset) const
{
    const auto paramName = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName()).value_or("").toStdString();
    if (!paramName.empty()) {
        const auto valueType = reader.attribute(Constants::NahdXml::xmlKeyParameterValueType()).toString();
        const auto xmlValue = reader.attribute(Constants::NahdXml::xmlKeyValue()).toString();

        if (valueType == Constants::NahdXml::xmlValueInt()) {
            preset.parameters[paramName] = static_cast<float>(xmlValue.toInt());
        } else if (valueType == Constants::NahdXml::xmlValueBool()) {
            preset.parameters[paramName] = (xmlValue == Constants::NahdXml::xmlValueTrue() || xmlValue == "1") ? 1.0f : 0.0f;
        } else if (valueType == Constants::NahdXml::xmlValueFloat()) {
            const auto xmlMin = reader.attribute(Constants::NahdXml::xmlKeyMin()).toInt();
            const auto xmlMax = reader.attribute(Constants::NahdXml::xmlKeyMax()).toInt();
            preset.parameters[paramName] = Parameter::xmlValueToInternal(xmlValue.toInt(), xmlMin, xmlMax);
        } else {
            // Fallback for older files
            preset.parameters[paramName] = legacyPresetParameterValue(reader, paramName, xmlValue);
        }
    }
    reader.skipCurrentElement();
}

SynthPreset DeviceService::deserializePreset(ProjectReader & reader) const
{
    const auto presetName = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName()).value_or("Init");
    SynthPreset preset { presetName.toStdString(), {} };

    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
            deserializePresetParameter(reader, preset);
        } else {
            reader.skipCurrentElement();
        }
    }
    return preset;
}

void DeviceService::deserializeUserPresets(ProjectReader & reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyPreset()) {
            const auto index = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyIndex()).value_or(0);
            m_synthUserPresets[index] = deserializePreset(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
    setSynthUserPresets(m_synthUserPresets);
}

void DeviceService::deserializeFromXml(ProjectReader & reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyDevice()) {
            deserializeDevice(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeySynth()) {
            // Handled via generic Device element if present in slot
        } else if (reader.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            deserializeMasterEffects(reader);
        } else if (reader.name() == Constants::NahdXml::xmlKeyUserPresets()) {
            deserializeUserPresets(reader);
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
    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);
    return exportDeviceSettings(slotIndex, writer);
}

bool DeviceService::exportDeviceSettings(int slotIndex, ProjectWriter & writer) const
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
    NahdXmlReader reader { file };
    return peekDeviceTypeInfo(reader);
}

DeviceService::DeviceTypeInfo DeviceService::peekDeviceTypeInfo(ProjectReader & reader) const
{
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                return {
                    reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString(),
                    reader.attribute(Constants::NahdXml::xmlKeyTypeName()).toString()
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
    NahdXmlReader reader { file };
    return importDeviceSettings(slotIndex, reader);
}

bool DeviceService::importDeviceSettings(int slotIndex, ProjectReader & reader)
{
    while (!reader.atEnd() && !reader.hasError()) {
        const auto token = reader.readNext();
        if (token == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeySettings()) {
                while (reader.readNextStartElement()) {
                    if (reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                        const auto typeId = reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString();
                        const auto deviceName = reader.attribute(Constants::NahdXml::xmlKeyName()).toString();

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
