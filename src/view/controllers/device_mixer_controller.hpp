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
class GainStagingService;

//! A device's channel strip, by rack slot: volume, gain, pan, where the fader sits and where the
//! sends tap.
class DeviceMixerController : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using GainStagingServiceS = std::shared_ptr<GainStagingService>;

    //! The slot Gain Stage is running on, -1 while it is not.
    Q_PROPERTY(int gainStagingSlot READ gainStagingSlot NOTIFY gainStagingSlotChanged)

    DeviceMixerController(DeviceServiceS deviceService, EditorServiceS editorService, GainStagingServiceS gainStagingService, QObject * parent = nullptr);
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
    Q_INVOKABLE int deviceWidth(int slotIndex) const;
    Q_INVOKABLE void setDeviceWidth(int slotIndex, int value);
    //! Device::FaderPosition and Device::SendTap as ints, for the Device Settings dialog.
    Q_INVOKABLE int deviceFaderPosition(int slotIndex) const;
    Q_INVOKABLE void setDeviceFaderPosition(int slotIndex, int value);
    Q_INVOKABLE int deviceSendTap(int slotIndex) const;
    Q_INVOKABLE void setDeviceSendTap(int slotIndex, int value);

    //! Gain Stage, held down on a strip: moves the slot's Gain towards the gain staging target until
    //! released.
    Q_INVOKABLE void startGainStaging(int slotIndex);
    Q_INVOKABLE void stopGainStaging();
    int gainStagingSlot() const;
    //! In dB, either way of the target
    Q_INVOKABLE double deviceGainStagingTrim(int slotIndex) const;
    Q_INVOKABLE void setDeviceGainStagingTrim(int slotIndex, double trimDb);
    //! Device::GainStagingMode as an int
    Q_INVOKABLE int deviceGainStagingMode(int slotIndex) const;
    Q_INVOKABLE void setDeviceGainStagingMode(int slotIndex, int value);

signals:
    void gainStagingSlotChanged();

private:
    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
    GainStagingServiceS m_gainStagingService;
};

} // namespace noteahead

#endif // DEVICE_MIXER_CONTROLLER_HPP
