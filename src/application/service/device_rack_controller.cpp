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

#include "device_rack_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "device_service.hpp"
#include "editor_service.hpp"
#include "sampler_controller.hpp"
#include "synth_controller.hpp"

namespace noteahead {

DeviceRackController::DeviceRackController(DeviceServiceS deviceService, SamplerControllerS samplerController, SynthControllerS synthController, EditorServiceS editorService, QObject * parent)
  : QAbstractListModel { parent }
  , m_deviceService { std::move(deviceService) }
  , m_samplerController { std::move(samplerController) }
  , m_synthController { std::move(synthController) }
  , m_editorService { std::move(editorService) }
{
    if (m_deviceService) {
        m_devices = m_deviceService->internalDeviceNamesQt();
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, &DeviceRackController::refresh);
    }
    if (m_editorService) {
        connect(m_editorService.get(), &EditorService::songChanged, this, &DeviceRackController::refresh);
    }
}

DeviceRackController::~DeviceRackController() = default;

int DeviceRackController::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_devices.size();
}

QVariant DeviceRackController::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto row = index.row();
    if (row < 0 || row >= m_devices.size()) {
        return {};
    }

    const auto & name = m_devices.at(row);

    switch (static_cast<DataRole>(role)) {
    case DataRole::Name:
        return name;
    case DataRole::TrackNames:
        return trackNames(name);
    }

    return {};
}

QHash<int, QByteArray> DeviceRackController::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { static_cast<int>(DataRole::Name), "name" },
        { static_cast<int>(DataRole::TrackNames), "trackNames" }
    };
    return roles;
}

void DeviceRackController::refresh()
{
    beginResetModel();
    if (m_deviceService) {
        m_devices = m_deviceService->internalDeviceNamesQt();
    }
    endResetModel();
}

QString DeviceRackController::trackNames(const QString & deviceName) const
{
    QStringList trackNames;
    for (const auto index : m_editorService->trackIndices()) {
        if (const auto portName = m_editorService->instrumentPortName(index); portName == deviceName) {
            trackNames << m_editorService->trackName(index);
        }
    }
    return trackNames.join(", ");
}

void DeviceRackController::openDevice(const QString & name)
{
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(m_deviceService->device(name.toStdString()))) {
        m_samplerController->setSampler(sampler);
        emit samplerDialogRequested();
    } else if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(m_deviceService->device(name.toStdString()))) {
        m_synthController->setSynth(synth);
        emit synthDialogRequested();
    }
}

} // namespace noteahead
