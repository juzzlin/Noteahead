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

#include "property_service.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/tracker/parameter_automation.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../note_converter.hpp"
#include "device_service.hpp"

#include <QCoreApplication>
#include <QVariantMap>

#include <algorithm>

#include <optional>

namespace noteahead {

namespace {
//! What a controller is worth on the wire. Anything wider only exists inside the application.
constexpr int midi1MinValue = 0;
constexpr int midi1MaxValue = 127;
} // namespace

PropertyService::PropertyService(QObject * parent)
  : QObject { parent }
{
}

void PropertyService::setDeviceService(std::weak_ptr<DeviceService> deviceService)
{
    m_deviceService = deviceService;
}

QVariantList PropertyService::availableMidiControllers() const
{
    return getAvailableMidiControllers();
}

namespace {

//! What the dialog knows a parameter by: its key, where its knob is, and how it reads out.
QVariantMap parameterMap(const std::string & key, const Parameter & parameter)
{
    QVariantMap map;
    map["key"] = QString::fromStdString(key);
    // Where the knob is set now, which is what a relative automation scales
    map["knob"] = static_cast<double>(parameter.authoredValue());
    if (const auto & display = parameter.display(); display) {
        map["label"] = display->label.empty() ? QString::fromStdString(key) : QCoreApplication::translate("ParameterDisplay", display->label.c_str());
        map["unit"] = QString::fromStdString(display->unit);
        map["mapping"] = QString::fromStdString(display->mappingName());
        map["min"] = display->min;
        map["max"] = display->max;
        map["rest"] = display->restPosition();
    } else {
        map["label"] = QString::fromStdString(key);
        map["unit"] = QString { "%" };
        map["mapping"] = QString { "linear" };
        map["min"] = 0.0;
        map["max"] = 100.0;
        map["rest"] = 0.0;
    }
    return map;
}

void sortByLabel(QVariantList & parameters)
{
    std::ranges::sort(parameters, [](auto && lhs, auto && rhs) {
        return QString::localeAwareCompare(lhs.toMap()["label"].toString(), rhs.toMap()["label"].toString()) < 0;
    });
}

//! Every continuous parameter of an effect, described or not.
QVariantList effectParameters(const Effect & effect)
{
    QVariantList parameters;
    for (auto && [key, parameter] : effect.parameters()) {
        if (parameter.type() == Parameter::Type::Continuous) {
            parameters.push_back(parameterMap(key, parameter));
        }
    }
    sortByLabel(parameters);
    return parameters;
}

//! A device's parameters, split by the pad they belong to: the device's own first, then a group for
//! each pad in note order. Listing a pad as a target of its own keeps each parameter list short.
struct DeviceParameterGroup
{
    std::optional<uint8_t> groupNote;
    std::string groupName;
    QVariantList parameters;
};

//! "Pad C-2 (kick.wav)" for a pad: the note it plays on, as the tracker writes it, and what is loaded.
//! A part named rather than played by note goes by its name, a drum say. Empty for the device's own.
QString groupLabel(const DeviceParameterGroup & group)
{
    const auto name = QString::fromStdString(group.groupName);
    if (!group.groupNote) {
        return name;
    }
    const auto pad = QCoreApplication::translate("PropertyService", "Pad %1").arg(QString::fromStdString(NoteConverter::midiToString(*group.groupNote)));
    return name.isEmpty() ? pad : QString { "%1 (%2)" }.arg(pad, name);
}

std::vector<DeviceParameterGroup> deviceParameterGroups(const Device & device)
{
    std::vector<DeviceParameterGroup> groups;
    for (auto && automatable : device.automatableParameters()) {
        if (groups.empty() || groups.back().groupNote != automatable.groupNote || groups.back().groupName != automatable.groupName) {
            groups.push_back({ automatable.groupNote, automatable.groupName, {} });
        }
        groups.back().parameters.push_back(parameterMap(automatable.key, automatable.parameter));
    }
    for (auto && group : groups) {
        sortByLabel(group.parameters);
    }
    return groups;
}

} // namespace

QVariantList PropertyService::automationTargets() const
{
    QVariantList targets;
    if (const auto deviceService = m_deviceService.lock(); deviceService) {
        for (size_t slot = 0; slot < Constants::deviceRackSize(); slot++) {
            if (const auto device = deviceService->device(slot); device) {
                for (auto && group : deviceParameterGroups(*device)) {
                    QVariantMap map;
                    map["owner"] = ParameterAutomation::Target::ownerToString(ParameterAutomation::Target::Owner::Device);
                    map["ownerId"] = QString::fromStdString(device->uuid());
                    map["deviceSlot"] = static_cast<int>(slot);
                    map["deviceTypeName"] = QString::fromStdString(device->typeName());
                    // A pad's parameters are a target of their own, named by the pad
                    map["group"] = groupLabel(group);
                    map["parameters"] = std::move(group.parameters);
                    targets.push_back(map);
                }
            }
        }
        for (auto && location : deviceService->effectLocations()) {
            QVariantMap map;
            map["owner"] = ParameterAutomation::Target::ownerToString(ParameterAutomation::Target::Owner::Effect);
            map["ownerId"] = QString::fromStdString(location.effect->id());
            map["typeId"] = QString::fromStdString(location.effect->typeId());
            map["type"] = QString::fromStdString(location.effect->type());
            map["rack"] = static_cast<int>(location.rack);
            map["deviceSlot"] = static_cast<int>(location.deviceSlot);
            map["deviceTypeName"] = QString::fromStdString(location.deviceTypeName);
            map["rackIndex"] = static_cast<int>(location.rackIndex);
            map["effectSlot"] = static_cast<int>(location.effectSlot);
            map["parameters"] = effectParameters(*location.effect);
            targets.push_back(map);
        }
    }
    return targets;
}

QVariantList PropertyService::automationTargetsForPort(const QString & portName) const
{
    QVariantList targets;
    const auto deviceService = m_deviceService.lock();
    if (!deviceService) {
        return targets;
    }
    const auto device = deviceService->findDevice(deviceService->deviceUuidForPort(portName).toStdString());
    if (!device) {
        return targets;
    }

    const auto deviceOwner = ParameterAutomation::Target::ownerToString(ParameterAutomation::Target::Owner::Device);
    const auto deviceUuid = QString::fromStdString(device->uuid());
    const auto deviceSlot = static_cast<int>(device->id());
    std::ranges::copy_if(automationTargets(), std::back_inserter(targets), [&](auto && target) {
        const auto map = target.toMap();
        if (map["owner"].toString() == deviceOwner) {
            return map["ownerId"].toString() == deviceUuid;
        }
        // The device's insert rack and the racks of its voices or pads; not the master or the sends
        const auto rack = static_cast<DeviceService::EffectLocation::Rack>(map["rack"].toInt());
        const bool isDeviceRack = rack == DeviceService::EffectLocation::Rack::DeviceInsert || rack == DeviceService::EffectLocation::Rack::DeviceVoice;
        return isDeviceRack && map["deviceSlot"].toInt() == deviceSlot;
    });
    return targets;
}

QVariantMap PropertyService::automationParameter(const QString & owner, const QString & ownerId, const QString & key) const
{
    const auto deviceService = m_deviceService.lock();
    if (!deviceService) {
        return {};
    }
    QVariantList parameters;
    if (ParameterAutomation::Target::ownerFromString(owner) == ParameterAutomation::Target::Owner::Device) {
        if (const auto device = deviceService->findDevice(ownerId.toStdString()); device) {
            for (auto && group : deviceParameterGroups(*device)) {
                parameters.append(group.parameters);
            }
        }
    } else if (const auto effect = deviceService->findEffect(ownerId.toStdString()); effect) {
        parameters = effectParameters(*effect);
    }
    for (auto && parameter : parameters) {
        if (const auto map = parameter.toMap(); map["key"].toString() == key) {
            return map;
        }
    }
    return {};
}

QVariantList PropertyService::getAvailableMidiControllers(const QString & portName) const
{
    using namespace MidiCcMapping;
    QVariantList list;

    const auto addController = [&](uint8_t i, const QString & customName = {}, int minValue = midi1MinValue, int maxValue = midi1MaxValue, std::optional<uint8_t> note = std::nullopt) {
        QString name;
        if (!customName.isEmpty()) {
            name = QString { "%1: %2" }.arg(i).arg(customName);
        } else {
            name = controllerToString(static_cast<Controller>(i));
            if (name == "Undefined") {
                name = QString { "%1" }.arg(i);
            } else {
                name = QString { "%1: %2" }.arg(i).arg(name);
            }
        }
        // A controller that drives one key or pad names its note, so the list reads against the tracker
        if (note.has_value()) {
            name += QString { " (%1)" }.arg(QString::fromStdString(NoteConverter::midiToString(note.value())));
        }
        list.append(QVariantMap {
          { "number", i },
          { "name", name },
          { "minValue", minValue },
          { "maxValue", maxValue } });
    };

    if (auto ds = m_deviceService.lock()) {
        if (auto dev = ds->device(portName.toStdString())) {
            // An internal device answers for its own ranges rather than being held to MIDI 1.0
            for (auto && controller : dev->availableMidiCcControllers()) {
                addController(controller.number, QString::fromStdString(controller.name), controller.minValue, controller.maxValue, controller.note);
            }
            return list;
        }
    }

    for (uint8_t i { 0 }; i < 128; ++i) {
        addController(i);
    }
    return list;
}

std::optional<MidiCcController> PropertyService::deviceController(int controller, const QString & portName) const
{
    if (portName.isEmpty()) {
        return std::nullopt;
    }
    if (const auto ds = m_deviceService.lock()) {
        if (const auto dev = ds->device(portName.toStdString())) {
            for (auto && available : dev->availableMidiCcControllers()) {
                if (available.number == controller) {
                    return available;
                }
            }
        }
    }
    return std::nullopt;
}

int PropertyService::minValue(int controller, const QString & portName) const
{
    if (const auto available = deviceController(controller, portName)) {
        return available->minValue;
    }
    return midi1MinValue;
}

int PropertyService::maxValue(int controller, const QString & portName) const
{
    if (const auto available = deviceController(controller, portName)) {
        return available->maxValue;
    }
    return midi1MaxValue;
}

} // namespace noteahead
