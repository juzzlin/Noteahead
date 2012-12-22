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

#include "midi_mapping_controller.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

namespace noteahead {

MidiMappingController::MidiMappingController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
{
}

MidiMappingController::~MidiMappingController() = default;

QVariantList MidiMappingController::deviceMidiMappings(int slotIndex) const
{
    QVariantList list;
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        for (auto && mapping : device->midiMappings()) {
            list.push_back(QVariantMap {
              { "controller", static_cast<int>(mapping.controller) },
              { "target", QString::fromStdString(mapping.target) },
              { "min", mapping.min },
              { "max", mapping.max } });
        }
    }
    return list;
}

void MidiMappingController::setDeviceMidiMappings(int slotIndex, const QVariantList & mappings)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        Device::MidiMappingList list;
        for (auto && item : mappings) {
            const auto map = item.toMap();
            MidiMapping mapping;
            mapping.controller = static_cast<uint16_t>(std::clamp(map["controller"].toInt(), 0, static_cast<int>(Device::MaxMappableCc)));
            mapping.target = map["target"].toString().toStdString();
            mapping.min = std::clamp(map["min"].toDouble(), 0.0, 1.0);
            mapping.max = std::clamp(map["max"].toDouble(), 0.0, 1.0);
            list.push_back(std::move(mapping));
        }
        if (list != device->midiMappings()) {
            device->setMidiMappings(std::move(list));
            m_editorService->setIsModified(true);
        }
    }
}

int MidiMappingController::maxMappableMidiCc() const
{
    return Device::MaxMappableCc;
}

void MidiMappingController::notifyControllerCcReceived(int controller)
{
    emit controllerCcReceived(controller);
}

} // namespace noteahead
