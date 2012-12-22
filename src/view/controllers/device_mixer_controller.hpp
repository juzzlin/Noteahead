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

#ifndef DEVICE_MIXER_CONTROLLER_HPP
#define DEVICE_MIXER_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace noteahead {

class DeviceService;
class EditorService;

//! A device's channel strip, by rack slot: volume, gain, pan, where the fader sits and where the
//! sends tap.
class DeviceMixerController : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    DeviceMixerController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);
    ~DeviceMixerController() override;

    //! Mixer values of the device in a slot, in the same 0..uiInternalScaling units
    //! DeviceDialogModel uses.
    //!
    //! Devices with a dedicated controller expose these through it; a Sub Mixer has none, so its
    //! dialog reads and writes them by slot instead.
    Q_INVOKABLE int deviceVolume(int slotIndex) const;
    Q_INVOKABLE void setDeviceVolume(int slotIndex, int value);
    Q_INVOKABLE int deviceGain(int slotIndex) const;
    Q_INVOKABLE void setDeviceGain(int slotIndex, int value);
    Q_INVOKABLE int devicePan(int slotIndex) const;
    Q_INVOKABLE void setDevicePan(int slotIndex, int value);
    //! Device::FaderPosition and Device::SendTap as ints, for the Device Settings dialog.
    Q_INVOKABLE int deviceFaderPosition(int slotIndex) const;
    Q_INVOKABLE void setDeviceFaderPosition(int slotIndex, int value);
    Q_INVOKABLE int deviceSendTap(int slotIndex) const;
    Q_INVOKABLE void setDeviceSendTap(int slotIndex, int value);

signals:

private:
    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // DEVICE_MIXER_CONTROLLER_HPP
