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
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QDateTime>
#include <QFile>
#include <QVariant>

#include <algorithm>
#include <format>
#include <ranges>
#include <set>

namespace noteahead {

static const auto TAG = "DeviceService";

DeviceService::DeviceService(AudioEngineS audioEngine, DataServiceS dataService, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
  , m_dataService { std::move(dataService) }
  , m_internalDevicePortPrefix { Constants::internalDevicePortPrefix() }
{
    m_deviceCache.resize(Constants::deviceRackSize());
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
    cacheDevice(slotIndex, device);
    m_audioEngine->setDevice(slotIndex, std::move(device));
    emit dataChanged();
}

void DeviceService::replaceDevice(size_t slotIndex, DeviceS device)
{
    if (const auto previous = this->device(slotIndex); previous && device) {
        device->adoptChannelStripFrom(*previous);
    }
    setDevice(slotIndex, std::move(device));
}

void DeviceService::clearDevice(size_t slotIndex)
{
    cacheDevice(slotIndex, nullptr);
    m_audioEngine->clearDevice(slotIndex);
    pruneSubMixerMembers();
    emit dataChanged();
}

bool DeviceService::wouldCreateCycle(size_t subMixerSlot, size_t memberSlot) const
{
    if (subMixerSlot == memberSlot) {
        return true;
    }

    // Walk up from the prospective SubMixer: if anything already reachable as one of memberSlot's
    // descendants is the SubMixer itself, adding the edge would close the loop.
    std::vector<size_t> pending { memberSlot };
    std::set<size_t> visited;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        if (!visited.insert(current).second) {
            continue;
        }
        if (current == subMixerSlot) {
            return true;
        }
        if (const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(current))) {
            for (const auto nested : sub->members()) {
                pending.push_back(nested);
            }
        }
    }
    return false;
}

void DeviceService::pruneSubMixerMembers()
{
    for (size_t slotIndex = 0; slotIndex < Constants::deviceRackSize(); slotIndex++) {
        const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(slotIndex));
        if (!sub) {
            continue;
        }
        auto members = sub->members();
        const auto removed = std::remove_if(members.begin(), members.end(), [this](size_t memberSlot) {
            return !device(memberSlot);
        });
        if (removed != members.end()) {
            members.erase(removed, members.end());
            sub->setMembers(std::move(members));
        }
    }
}

bool DeviceService::canAddSubMixerMember(int subMixerSlot, int memberSlot) const
{
    if (subMixerSlot < 0 || memberSlot < 0) {
        return false;
    }

    const auto subSlot = static_cast<size_t>(subMixerSlot);
    const auto slot = static_cast<size_t>(memberSlot);
    return std::dynamic_pointer_cast<SubMixerDevice>(device(subSlot)) && device(slot) && !wouldCreateCycle(subSlot, slot);
}

bool DeviceService::addSubMixerMember(int subMixerSlot, int memberSlot)
{
    if (!canAddSubMixerMember(subMixerSlot, memberSlot)) {
        return false;
    }

    const auto subSlot = static_cast<size_t>(subMixerSlot);
    const auto slot = static_cast<size_t>(memberSlot);
    const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(subSlot));

    // Exclusive membership: being summed by two SubMixers would play the device twice.
    for (size_t other = 0; other < Constants::deviceRackSize(); other++) {
        if (other == subSlot) {
            continue;
        }
        if (const auto otherSub = std::dynamic_pointer_cast<SubMixerDevice>(device(other))) {
            auto members = otherSub->members();
            const auto removed = std::remove(members.begin(), members.end(), slot);
            if (removed != members.end()) {
                members.erase(removed, members.end());
                otherSub->setMembers(std::move(members));
            }
        }
    }

    auto members = sub->members();
    if (std::ranges::find(members, slot) == members.end()) {
        members.push_back(slot);
        sub->setMembers(std::move(members));
    }

    emit dataChanged();
    return true;
}

bool DeviceService::removeSubMixerMember(int subMixerSlot, int memberSlot)
{
    if (subMixerSlot < 0 || memberSlot < 0) {
        return false;
    }

    const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(static_cast<size_t>(subMixerSlot)));
    if (!sub) {
        return false;
    }

    auto members = sub->members();
    const auto removed = std::remove(members.begin(), members.end(), static_cast<size_t>(memberSlot));
    if (removed == members.end()) {
        return false;
    }
    members.erase(removed, members.end());
    sub->setMembers(std::move(members));

    emit dataChanged();
    return true;
}

QVariantList DeviceService::subMixerMembers(int subMixerSlot) const
{
    QVariantList result;
    if (subMixerSlot < 0) {
        return result;
    }
    if (const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(static_cast<size_t>(subMixerSlot)))) {
        for (const auto slot : sub->members()) {
            result.append(static_cast<int>(slot));
        }
    }
    return result;
}

int DeviceService::subMixerOwningSlot(int memberSlot) const
{
    if (memberSlot < 0) {
        return -1;
    }
    for (size_t slotIndex = 0; slotIndex < Constants::deviceRackSize(); slotIndex++) {
        if (const auto sub = std::dynamic_pointer_cast<SubMixerDevice>(device(slotIndex))) {
            const auto members = sub->members();
            if (std::ranges::find(members, static_cast<size_t>(memberSlot)) != members.end()) {
                return static_cast<int>(slotIndex);
            }
        }
    }
    return -1;
}

void DeviceService::cacheDevice(size_t slotIndex, DeviceS device)
{
    std::lock_guard<std::mutex> lock { m_deviceCacheMutex };
    if (slotIndex < m_deviceCache.size()) {
        m_deviceCache.at(slotIndex) = std::move(device);
    }
}

void DeviceService::clearDeviceCache()
{
    std::lock_guard<std::mutex> lock { m_deviceCacheMutex };
    std::ranges::fill(m_deviceCache, nullptr);
}

std::optional<size_t> DeviceService::slotFromPortName(const QString & portName) const
{
    if (!portName.startsWith(m_internalDevicePortPrefix)) {
        return std::nullopt;
    }
    // "<prefix> <n>", with the slot numbered from one on the wire and from zero in the rack.
    bool converted = false;
    const auto slot = QStringView { portName }.mid(m_internalDevicePortPrefix.length() + 1).toUInt(&converted);
    if (!converted || !slot) {
        return std::nullopt;
    }
    return static_cast<size_t>(slot - 1);
}

DeviceService::DeviceS DeviceService::deviceForPort(const QString & portName) const
{
    if (const auto slotIndex = slotFromPortName(portName); slotIndex.has_value()) {
        return device(*slotIndex);
    }
    return device(portName.toStdString());
}

DeviceService::DeviceS DeviceService::device(size_t slotIndex) const
{
    std::lock_guard<std::mutex> lock { m_deviceCacheMutex };
    return slotIndex < m_deviceCache.size() ? m_deviceCache.at(slotIndex) : nullptr;
}

DeviceService::DeviceS DeviceService::device(const std::string & name) const
{
    if (const auto slotIndex = slotFromPortName(QString::fromStdString(name)); slotIndex.has_value()) {
        return device(*slotIndex);
    }

    // A device renamed away from the slot pattern is still addressable by the name it carries.
    std::lock_guard<std::mutex> lock { m_deviceCacheMutex };
    const auto it = std::ranges::find_if(m_deviceCache, [&name](const auto & device) {
        return device && device->name() == name;
    });
    return it != m_deviceCache.end() ? *it : nullptr;
}

bool DeviceService::isInternalDevice(const QString & portName) const
{
    return portName.startsWith(m_internalDevicePortPrefix);
}

void DeviceService::processMidiNoteOn(const QString & portName, uint8_t note, uint8_t velocity)
{
    if (const auto dev = deviceForPort(portName); dev) {
        dev->processMidiNoteOn(note, velocity);
    }
}

std::optional<uint64_t> DeviceService::frameForTime(std::chrono::steady_clock::time_point time) const
{
    if (!m_audioEngine) {
        return std::nullopt;
    }
    const auto anchor = m_audioEngine->frameAnchor();
    if (!anchor.running || !anchor.sampleRate) {
        return std::nullopt;
    }

    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
    const double seconds = static_cast<double>(nanoseconds - anchor.nanoseconds) / 1e9;
    const auto offset = static_cast<int64_t>(seconds * anchor.sampleRate);
    if (const auto frame = static_cast<int64_t>(anchor.frame) + offset; frame > 0) {
        return static_cast<uint64_t>(frame);
    }
    return std::nullopt;
}

std::optional<std::chrono::steady_clock::duration> DeviceService::scheduleLookahead() const
{
    if (!m_audioEngine) {
        return std::nullopt;
    }
    const auto anchor = m_audioEngine->frameAnchor();
    if (!anchor.running || !anchor.sampleRate || !anchor.blockFrames) {
        return std::nullopt;
    }

    // Two things have to be cleared. How far apart the engine's chances to start a note are, which
    // is the gap between blocks and is a whole burst on a backend that delivers them together; and
    // how much audio the backend is holding, which the stream is asked to keep at two blocks. The
    // larger wins, and a margin on top covers the jitter in getting here at all.
    const auto blockNanoseconds = static_cast<int64_t>(1e9 * anchor.blockFrames / anchor.sampleRate);
    const auto needed = std::max(anchor.maxBlockGapNanoseconds, blockNanoseconds * Constants::pulseLatencyBufferCount());
    const auto margin = static_cast<int64_t>(Constants::playbackScheduleMarginMs()) * 1000000;
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::nanoseconds { needed + margin });
}

void DeviceService::scheduleMidiNoteOn(const QString & portName, uint8_t note, uint8_t velocity, uint64_t frame)
{
    if (const auto dev = deviceForPort(portName); dev) {
        Device::ScheduledEvent event;
        event.type = Device::ScheduledEvent::Type::NoteOn;
        event.frame = frame;
        event.note = note;
        event.velocity = velocity;
        dev->scheduleMidiEvent(event);
    }
}

void DeviceService::scheduleMidiNoteOff(const QString & portName, uint8_t note, uint64_t frame)
{
    if (const auto dev = deviceForPort(portName); dev) {
        Device::ScheduledEvent event;
        event.type = Device::ScheduledEvent::Type::NoteOff;
        event.frame = frame;
        event.note = note;
        dev->scheduleMidiEvent(event);
    }
}

void DeviceService::scheduleMidiCc(const QString & portName, uint8_t controller, uint8_t value, uint8_t channel, uint64_t frame)
{
    if (const auto dev = deviceForPort(portName); dev) {
        Device::ScheduledEvent event;
        event.type = Device::ScheduledEvent::Type::Cc;
        event.frame = frame;
        event.controller = controller;
        event.velocity = value;
        event.channel = channel;
        dev->scheduleMidiEvent(event);
    }
}

void DeviceService::scheduleMidiPitchBend(const QString & portName, uint16_t value, uint8_t channel, uint64_t frame)
{
    if (const auto dev = deviceForPort(portName); dev) {
        Device::ScheduledEvent event;
        event.type = Device::ScheduledEvent::Type::PitchBend;
        event.frame = frame;
        event.value = value;
        event.channel = channel;
        dev->scheduleMidiEvent(event);
    }
}

void DeviceService::clearScheduledEvents()
{
    const std::lock_guard<std::mutex> lock { m_deviceCacheMutex };
    for (auto && device : m_deviceCache) {
        if (device) {
            device->clearScheduledEvents();
        }
    }
}

void DeviceService::processMidiNoteOff(const QString & portName, uint8_t note)
{
    if (const auto dev = deviceForPort(portName); dev) {
        dev->processMidiNoteOff(note);
    }
}

void DeviceService::processMidiCc(const QString & portName, uint8_t controller, uint8_t value, uint8_t channel)
{
    if (const auto dev = deviceForPort(portName); dev) {
        dev->processMidiCc(controller, value, channel);
    }
}

void DeviceService::processMidiPitchBend(const QString & portName, uint16_t value, uint8_t channel)
{
    if (const auto dev = deviceForPort(portName); dev) {
        dev->processMidiPitchBend(value, channel);
    }
}

void DeviceService::processMidiProgramChange(const QString & portName, uint8_t program, uint8_t channel)
{
    if (const auto dev = deviceForPort(portName); dev) {
        dev->processMidiProgramChange(program, channel);
    }
}

void DeviceService::processMidiAllNotesOff(const QString & portName)
{
    if (const auto dev = deviceForPort(portName); dev) {
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

void DeviceService::applyInstrumentPatch(const Instrument & instrument)
{
    const auto portName = instrument.midiAddress().portName();
    if (!isInternalDevice(portName) || !instrument.settings().patch.has_value()) {
        return;
    }
    processMidiProgramChange(portName, *instrument.settings().patch, instrument.midiAddress().channel());
}

void DeviceService::applyInstrumentMidiCcSettings(const Instrument & instrument)
{
    const auto portName = instrument.midiAddress().portName();
    if (!isInternalDevice(portName)) {
        return;
    }

    const auto channel = instrument.midiAddress().channel();
    processMidiCc(portName, static_cast<uint8_t>(MidiCcMapping::Controller::ResetAllControllers), 127, channel);
    for (auto && midiCcSetting : instrument.settings().midiCcSettings) {
        if (midiCcSetting.enabled()) {
            processMidiCc(portName, static_cast<uint8_t>(midiCcSetting.controller()), static_cast<uint8_t>(midiCcSetting.value()), channel);
        }
    }
}

void DeviceService::applyInstrumentSettings(const Instrument & instrument)
{
    applyInstrumentPatch(instrument);
    applyInstrumentMidiCcSettings(instrument);
}

void DeviceService::clearAutomation()
{
    for (const auto & name : internalDeviceNames()) {
        if (const auto dev = device(name)) {
            dev->clearAutomation();
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
    // Rack-level enabled flag from the <SendEffects> element (defaults to enabled for older projects).
    m_audioEngine->sendEffectRack().setEnabled(reader.attribute(Constants::NahdXml::xmlKeyEnabled()).toString() != Constants::NahdXml::xmlValueFalse());
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
    return importDeviceSettingsFromXml(slotIndex, QString::fromUtf8(file.readAll()));
}

bool DeviceService::importDeviceSettingsFromXml(int slotIndex, const QString & xml)
{
    // Extract embedded data before deserializing the device so that a Sampler can resolve its
    // nahd:// sample paths while loading. In the file the <Device> element precedes the <Data>
    // blocks, so a single streaming pass would try to load samples before they are extracted.
    // extractData() appends to the already-extracted data (it does not clear), so any embedded
    // samples of a currently loaded project are preserved.
    NahdXmlReader dataReader { xml };
    while (!dataReader.atEnd()) {
        if (dataReader.isStartElement() && dataReader.name() == Constants::NahdXml::xmlKeyData()) {
            m_dataService->extractData(dataReader);
        }
        dataReader.readNext();
    }

    NahdXmlReader reader { xml };
    return importDeviceSettings(slotIndex, reader);
}

bool DeviceService::copyDevice(int sourceSlot, int targetSlot)
{
    if (sourceSlot == targetSlot || !device(static_cast<size_t>(sourceSlot))) {
        return false;
    }

    QString xml;
    {
        NahdXmlWriter writer { xml };
        if (!exportDeviceSettings(sourceSlot, writer)) {
            return false;
        }
    }

    return importDeviceSettingsFromXml(targetSlot, xml);
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

                        auto dev = device(static_cast<size_t>(slotIndex));
                        // Create a device when the slot is empty or holds a device of a different type. The slot's
                        // canonical name is used so the imported device integrates as this slot's device regardless of
                        // the name stored in the file.
                        if (!dev || dev->typeId() != typeId.toStdString()) {
                            const auto slotName = Constants::internalDevicePortPrefix().toStdString() + " " + std::to_string(slotIndex + 1);
                            if (typeId.toStdString() == SamplerDevice::typeIdString() && m_samplerAudioFileReaderFactory) {
                                dev = std::make_shared<SamplerDevice>(slotName, m_samplerAudioFileReaderFactory());
                            } else {
                                dev = DeviceFactory::createDevice(typeId.toStdString(), slotName);
                            }
                            if (dev) {
                                setDevice(static_cast<size_t>(slotIndex), dev);
                                // Re-acquire dev because setDevice moved it
                                dev = device(static_cast<size_t>(slotIndex));
                            }
                        }

                        if (dev) {
                            try {
                                dev->deserializeFromXml(reader);
                                dev->setId(static_cast<size_t>(slotIndex));
                            } catch (const std::exception & e) {
                                // Deserialization can throw (e.g. a Sampler failing to load a sample). This is
                                // invoked from QML, so swallow the exception here to fail gracefully instead of
                                // crossing the C++/QML boundary and crashing.
                                juzzlin::L(TAG).error() << std::format("Failed to import device settings: {}", e.what());
                                return false;
                            }
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
    clearDeviceCache();
    m_audioEngine->clear();
    emit dataChanged();
}

void DeviceService::resetLoudnessMeters()
{
    const auto resetRack = [](const EffectRack & rack) {
        for (const auto & effect : rack.effects()) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                meter->requestReset();
            }
        }
    };

    resetRack(m_audioEngine->sendEffectRack());
    resetRack(m_audioEngine->insertEffectRack());
    for (size_t slotIndex = 0; slotIndex < Constants::deviceRackSize(); slotIndex++) {
        if (const auto device = this->device(slotIndex)) {
            resetRack(device->insertEffectRack());
        }
    }
}

EffectRack & DeviceService::sendEffectRack()
{
    return m_audioEngine->sendEffectRack();
}

EffectRack & DeviceService::insertEffectRack()
{
    return m_audioEngine->insertEffectRack();
}

DeviceService::AudioEngineS DeviceService::audioEngine() const
{
    return m_audioEngine;
}

} // namespace noteahead
