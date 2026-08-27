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

#ifndef DEVICE_RACK_CONTROLLER_HPP
#define DEVICE_RACK_CONTROLLER_HPP

#include <QAbstractListModel>
#include <QObject>
#include <QStringList>
#include <QUrl>

#include <memory>
#include <vector>

#include "device_controller.hpp"

namespace noteahead {

class DeviceService;
class EditorService;

class DeviceRackController : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int deviceCount READ deviceCount NOTIFY deviceCountChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum class DataRole
    {
        Name = Qt::UserRole + 1,
        TrackNames,
        TypeName,
        TypeId
    };
    Q_ENUM(DataRole)

    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using DeviceControllerS = DeviceController::DeviceControllerS;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using ControllerList = std::vector<DeviceControllerS>;
    explicit DeviceRackController(DeviceServiceS deviceService, ControllerList controllers, EditorServiceS editorService, QObject * parent = nullptr);
    ~DeviceRackController() override;

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int deviceCount() const;
    int revision() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openDevice(const QString & name);
    Q_INVOKABLE void openDevice(int slotIndex);
    Q_INVOKABLE void requestEffectSendsDialog(const QString & deviceName);
    Q_INVOKABLE void requestDeviceSettingsDialog(const QString & deviceName);

    Q_INVOKABLE void setDevice(int slotIndex, const QString & typeId);
    //! Puts a new device in the slot but keeps the channel strip the old one had. What the Device
    //! Gallery calls, so that changing the instrument does not take the mix around it along.
    Q_INVOKABLE void replaceDevice(int slotIndex, const QString & typeId);
    Q_INVOKABLE void clearDevice(int slotIndex);

    Q_INVOKABLE void exportSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void importSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void confirmImportSettings(int index, const QUrl & fileUrl);

    Q_INVOKABLE void copyDevice(int sourceSlot, int targetSlot);
    Q_INVOKABLE QVariantList populatedDevices() const;

    Q_INVOKABLE QString deviceType(int slotIndex) const;
    Q_INVOKABLE QString deviceTypeName(int slotIndex) const;
    Q_INVOKABLE QString deviceName(int slotIndex) const;
    Q_INVOKABLE QString trackNames(int slotIndex) const;
    Q_INVOKABLE QVariantList availableDevices() const;

    //! Every occupied slot a SubMixer could take, with whether it already belongs to one.
    //!
    //! Entries carry "owner" so the dialog can show a device as claimed elsewhere rather than
    //! silently stealing it, and "blocked" for slots that would close a routing loop.
    Q_INVOKABLE QVariantList subMixerCandidates(int subMixerSlot) const;
    //! Mixer values of the device in a slot, in the same 0..uiInternalScaling units
    //! DeviceController uses.
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

    //! Slot index of a device by name, or -1 when no such device is in the rack.
    Q_INVOKABLE int slotOfDevice(const QString & deviceName) const;

    //! [peakDb, rmsDb] of the device's pre-insert level tap. Empty when the slot is empty.
    Q_INVOKABLE QVariantList deviceMeterLevels(int slotIndex) const;
    //! Gates every device's level and load tap. Keep it enabled only while meters are on screen.
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

    Q_INVOKABLE bool addSubMixerMember(int subMixerSlot, int memberSlot);
    Q_INVOKABLE bool removeSubMixerMember(int subMixerSlot, int memberSlot);

    Q_INVOKABLE void addSampler();
    Q_INVOKABLE void addSynth();
    Q_INVOKABLE void addWavetableSynth();
    Q_INVOKABLE void addBassSynth();
    Q_INVOKABLE void addDrumSynth();
    Q_INVOKABLE void addPianoSynth();
    Q_INVOKABLE void addPianoSynthV2();
    Q_INVOKABLE void addPianoSynthV3();
    Q_INVOKABLE void addKick808();
    Q_INVOKABLE void addStringVoice();
    Q_INVOKABLE void addSubMixer();
    Q_INVOKABLE void removeDevice(const QString & name);

signals:
    void deviceCountChanged();
    void revisionChanged();
    void importSettingsConfirmationRequested(int slotIndex, QUrl fileUrl, QString currentTypeName, QString importedTypeName, bool typeMismatch);
    void samplerDialogRequested();
    void synthDialogRequested();
    void wavetableSynthDialogRequested();
    void bassSynthDialogRequested();
    void drumSynthDialogRequested();
    void pianoSynthDialogRequested();
    void pianoSynthV2DialogRequested();
    void pianoSynthV3DialogRequested();
    void kick808DialogRequested();
    void stringVoiceDialogRequested();
    void stringVoiceV2DialogRequested();
    void stringEnsembleDialogRequested();
    void deviceSettingsDialogRequested(QString deviceName);
    void subMixerDialogRequested(int slotIndex);
    void effectSendsDialogRequested(const QString & deviceName);

private:
    QString trackNames(const QString & deviceName) const;

    //! Push the current gate onto every device's taps. Needed after any slot change, because a
    //! freshly created device starts with its meters off regardless of what is on screen.
    void applyMetersActive();

    DeviceServiceS m_deviceService;
    ControllerList m_controllers;
    EditorServiceS m_editorService;

    QStringList m_devices;
    int m_revision { 0 };
    bool m_metersActive { false };
};

} // namespace noteahead

#endif // DEVICE_RACK_CONTROLLER_HPP
