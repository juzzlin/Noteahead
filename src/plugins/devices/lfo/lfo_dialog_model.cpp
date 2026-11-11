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

#include "lfo_dialog_model.hpp"

#include "../../../application/service/device_service.hpp"
#include "lfo_device.hpp"
#include "lfo_panel_controller.hpp"

namespace noteahead {

LfoDialogModel::LfoDialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DeviceDialogModel { parent }
  , m_deviceService { std::move(deviceService) }
{
    for (size_t lfo = 0; lfo < LfoDevice::LfoCount; lfo++) {
        m_panels.push_back(new LfoPanelController { lfo, *this });
    }
}

DeviceDialogModel::DeviceS LfoDialogModel::device() const
{
    return m_device;
}

bool LfoDialogModel::setDevice(DeviceS device)
{
    if (const auto lfoDevice = std::dynamic_pointer_cast<LfoDevice>(device)) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = lfoDevice;
        connectDeviceSignals();
        requestSettings();
        return true;
    }
    return false;
}

void LfoDialogModel::setDevice(const QString & deviceName)
{
    if (m_deviceService) {
        setDevice(m_deviceService->device(deviceName.toStdString()));
    }
}

QVariantList LfoDialogModel::lfos() const
{
    QVariantList list;
    for (auto * panel : m_panels) {
        list << QVariant::fromValue(static_cast<QObject *>(panel));
    }
    return list;
}

QStringList LfoDialogModel::waveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

int LfoDialogModel::maxRateDivider() const
{
    return Lfo::MaxRateDivider;
}

std::shared_ptr<LfoDevice> LfoDialogModel::lfoDevice() const
{
    return m_device;
}

const std::shared_ptr<DeviceService> & LfoDialogModel::deviceService() const
{
    return m_deviceService;
}

void LfoDialogModel::requestSettings()
{
    // Everything that changes an LFO arrives here: the panels through the device's dataChanged, and
    // so do automation, presets and a project load
    for (auto * panel : m_panels) {
        emit panel->changed();
    }
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
