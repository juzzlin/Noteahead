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

#include "xy_pad_controller.hpp"

#include "../../../application/service/device_service.hpp"
#include "xy_pad_device.hpp"
#include "xy_pad_panel_controller.hpp"

namespace noteahead {

XyPadController::XyPadController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DeviceController { parent }
  , m_deviceService { std::move(deviceService) }
{
    for (size_t pad = 0; pad < XyPadDevice::PadCount; pad++) {
        m_panels.push_back(new XyPadPanelController { pad, *this });
    }
}

DeviceController::DeviceS XyPadController::device() const
{
    return m_device;
}

bool XyPadController::setDevice(DeviceS device)
{
    if (const auto xyPadDevice = std::dynamic_pointer_cast<XyPadDevice>(device)) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = xyPadDevice;
        connectDeviceSignals();
        connect(m_device.get(), &XyPadDevice::padPressed, this, &XyPadController::padPressed);
        connect(m_device.get(), &Device::parameterAutomationScheduled, this, &XyPadController::padsAutomated);
        requestSettings();
        return true;
    }
    return false;
}

void XyPadController::setDevice(const QString & deviceName)
{
    if (m_deviceService) {
        setDevice(m_deviceService->device(deviceName.toStdString()));
    }
}

QVariantList XyPadController::pads() const
{
    QVariantList list;
    for (auto * panel : m_panels) {
        list << QVariant::fromValue(static_cast<QObject *>(panel));
    }
    return list;
}

int XyPadController::padCount() const
{
    return static_cast<int>(XyPadDevice::PadCount);
}

void XyPadController::press(int pad, double x, double y)
{
    if (m_device && pad >= 0) {
        m_device->press(static_cast<size_t>(pad), static_cast<float>(x), static_cast<float>(y));
    }
}

void XyPadController::move(int pad, double x, double y)
{
    if (m_device && pad >= 0) {
        m_device->move(static_cast<size_t>(pad), static_cast<float>(x), static_cast<float>(y));
    }
}

void XyPadController::release(int pad)
{
    if (m_device && pad >= 0) {
        m_device->release(static_cast<size_t>(pad));
    }
}

double XyPadController::positionX(int pad) const
{
    return m_device && pad >= 0 ? static_cast<double>(m_device->positionX(static_cast<size_t>(pad))) : 0.0;
}

double XyPadController::positionY(int pad) const
{
    return m_device && pad >= 0 ? static_cast<double>(m_device->positionY(static_cast<size_t>(pad))) : 0.0;
}

bool XyPadController::isPressed(int pad) const
{
    return m_device && pad >= 0 && m_device->isPressed(static_cast<size_t>(pad));
}

bool XyPadController::isMoving(int pad) const
{
    return m_device && pad >= 0 && m_device->isMoving(static_cast<size_t>(pad));
}

std::shared_ptr<XyPadDevice> XyPadController::xyPadDevice() const
{
    return m_device;
}

const std::shared_ptr<DeviceService> & XyPadController::deviceService() const
{
    return m_deviceService;
}

void XyPadController::requestSettings()
{
    // Everything that changes a pad's settings arrives here, through the device's dataChanged
    for (auto * panel : m_panels) {
        emit panel->changed();
    }
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
