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
#include <QVariantMap>

#include <memory>
#include <vector>

#include "automation_refresh_throttle.hpp"
#include "device_dialog_model.hpp"

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
    using DeviceDialogModelS = DeviceDialogModel::DeviceDialogModelS;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using DialogModelList = std::vector<DeviceDialogModelS>;
    explicit DeviceRackController(DeviceServiceS deviceService, DialogModelList controllers, EditorServiceS editorService, QObject * parent = nullptr);
    ~DeviceRackController() override;

    //! Hands the user's preset store to every device controller. Done here rather than one line per
    //! controller in Application, so that a device added later cannot be left out of it.
    void setPresetService(DeviceDialogModel::PresetServiceS presetService);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int deviceCount() const;
    int revision() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openDevice(const QString & name);
    Q_INVOKABLE void openDevice(int slotIndex);

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
    //! False for a device with no sound of its own, whose mixer strip, sends and inserts are hidden.
    Q_INVOKABLE bool deviceProducesAudio(int slotIndex) const;
    Q_INVOKABLE QString deviceTypeName(int slotIndex) const;
    Q_INVOKABLE QString deviceName(int slotIndex) const;
    Q_INVOKABLE QString trackNames(int slotIndex) const;
    //! What the Device Rack shows beside a device: the tracks it plays on, or for a SubMixer the
    //! tracks its members play on.
    Q_INVOKABLE QString usageSummary(int slotIndex) const;
    Q_INVOKABLE QVariantList availableDevices() const;
    //! Every device the gallery offers, by the name it is shown under: the one list both the gallery
    //! and the dialogs' titles read, so a device's name is set in one place.
    static QVariantList deviceTypes();
    //! The name a device type is shown under, "Bass Synth", or empty for a type not in the gallery.
    static QString displayNameOf(const QString & typeId);
    //! The name the device in a slot is shown under, or its type's internal name for one the
    //! gallery does not list.
    Q_INVOKABLE QString deviceDisplayName(int slotIndex) const;

    //! Every occupied slot a SubMixer could take, with whether it already belongs to one.
    //!
    //! Entries carry "owner" so the dialog can show a device as claimed elsewhere rather than
    //! silently stealing it, and "blocked" for slots that would close a routing loop.
    Q_INVOKABLE QVariantList subMixerCandidates(int subMixerSlot) const;

    //! Slot index of a device by name, or -1 when no such device is in the rack.
    Q_INVOKABLE int slotOfDevice(const QString & deviceName) const;

    Q_INVOKABLE bool addSubMixerMember(int subMixerSlot, int memberSlot);
    Q_INVOKABLE bool removeSubMixerMember(int subMixerSlot, int memberSlot);

signals:
    void deviceCountChanged();
    void revisionChanged();
    void importSettingsConfirmationRequested(int slotIndex, QUrl fileUrl, QString currentTypeName, QString importedTypeName, bool typeMismatch);
    //! A device dialog open on the slot is bound to the device object that was there before the
    //! import, which a file of a different type replaces outright, so it has to react to this.
    void deviceSettingsImported(int slotIndex);
    //! A device that registers itself asks for its dialog by file name. See PluginCatalog.
    void pluginDeviceDialogRequested(QString dialogFile);
    void subMixerDialogRequested(int slotIndex);

private:
    QString trackNames(const QString & deviceName) const;
    QStringList trackNameList(int slotIndex) const;
    QStringList usageTrackNames(int slotIndex) const;

    DeviceServiceS m_deviceService;
    DialogModelList m_dialogModels;
    EditorServiceS m_editorService;

    QStringList m_devices;
    int m_revision { 0 };

    //! A drag or a busy MIDI controller changes a device dozens of times a second, and every change
    //! re-reads each binding on the revision and each row of the rack. The device takes the value at
    //! once; what is shown catches up at most once a frame.
    AutomationRefreshThrottle m_dataChangedRefresh;
};

} // namespace noteahead

#endif // DEVICE_RACK_CONTROLLER_HPP
