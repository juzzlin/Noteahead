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

#ifndef DEVICE_METERS_CONTROLLER_HPP
#define DEVICE_METERS_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace noteahead {

class DeviceService;
class EditorService;

//! What the device meters show, by rack slot: levels, loudness, clip lights and processing load,
//! and the gate that keeps the taps off while nothing shows them.
class DeviceMetersController : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    DeviceMetersController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);
    ~DeviceMetersController() override;

    //! [peakDb, rmsDb] of the device's pre-insert level tap. Empty when the slot is empty.
    Q_INVOKABLE QVariantList deviceMeterLevels(int slotIndex) const;
    //! Everything the mixer's OUT region can show, taken together so that every view of it describes
    //! the same moment: peakDb and rmsDb of the output level tap, shortTermLufs and integratedLufs
    //! of the output loudness tap. Empty when the slot is empty.
    //!
    //! A map rather than a list so that a metric added later is a new key, not a renumbering of
    //! every index the QML reads.
    Q_INVOKABLE QVariantMap deviceOutputMeters(int slotIndex) const;
    //! Starts the integrated measurement over, for one device or for the whole rack.
    Q_INVOKABLE void resetDeviceLoudness(int slotIndex);
    Q_INVOKABLE void resetAllDeviceLoudness();
    //! Gates every device's level, loudness and load tap. Keep it enabled only while meters are on
    //! screen. Counted per view: each view that turns them on must turn them off, and they stay on
    //! while any view still has them on, as when the mixer opens while the rack is still closing.
    Q_INVOKABLE void setMetersActive(bool active);
    //! Whether the device's output has hit full scale since the indicator was last cleared. Latches
    //! regardless of whether meters are on screen, so it also reports clipping you were not watching.
    Q_INVOKABLE bool deviceClipped(int slotIndex) const;
    Q_INVOKABLE void clearDeviceClip(int slotIndex);
    Q_INVOKABLE void clearAllDeviceClips();
    //! Share of the audio callback's real-time budget this device takes, in percent.
    Q_INVOKABLE double deviceLoad(int slotIndex) const;
    //! Whole-engine load in percent, and how many buffers have overrun since metering started.
    Q_INVOKABLE double totalLoad() const;
    Q_INVOKABLE double totalPeakLoad() const;
    Q_INVOKABLE int overrunCount() const;

signals:

private:
    //! Push the current gate onto every device's taps. Needed after any slot change, because a
    //! freshly created device starts with its meters off regardless of what is on screen.
    void applyMetersActive();
    int slotCount() const;
    //! Re-applies the gate when the devices in the rack change, since a new device's taps start off.
    void updateDevices();

    DeviceServiceS m_deviceService;
    QStringList m_devices;
    bool m_metersActive { false };
    //! How many views have the meters on
    int m_meterViewCount { 0 };
};

} // namespace noteahead

#endif // DEVICE_METERS_CONTROLLER_HPP
