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

#include <memory>

namespace noteahead {

class DeviceService;
class BassSynthController;
class DrumSynthController;
class EditorService;
class SamplerController;
class SynthController;

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
    using SamplerControllerS = std::shared_ptr<SamplerController>;
    using SynthControllerS = std::shared_ptr<SynthController>;
    using BassSynthControllerS = std::shared_ptr<BassSynthController>;
    using DrumSynthControllerS = std::shared_ptr<DrumSynthController>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    explicit DeviceRackController(DeviceServiceS deviceService, SamplerControllerS samplerController, SynthControllerS synthController, BassSynthControllerS bassSynthController, DrumSynthControllerS drumSynthController, EditorServiceS editorService, QObject * parent = nullptr);
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

    Q_INVOKABLE void setDevice(int slotIndex, const QString & typeId);
    Q_INVOKABLE void clearDevice(int slotIndex);

    Q_INVOKABLE QString deviceType(int slotIndex) const;
    Q_INVOKABLE QString deviceTypeName(int slotIndex) const;
    Q_INVOKABLE QString deviceName(int slotIndex) const;
    Q_INVOKABLE QString trackNames(int slotIndex) const;
    Q_INVOKABLE QVariantList availableDevices() const;

    Q_INVOKABLE void addSampler();
    Q_INVOKABLE void addSynth();
    Q_INVOKABLE void addBassSynth();
    Q_INVOKABLE void addDrumSynth();
    Q_INVOKABLE void removeDevice(const QString & name);

signals:
    void deviceCountChanged();
    void revisionChanged();
    void samplerDialogRequested();
    void synthDialogRequested();
    void bassSynthDialogRequested();
    void drumSynthDialogRequested();
    void effectSendsDialogRequested(const QString & deviceName);

private:
    QString trackNames(const QString & deviceName) const;

    DeviceServiceS m_deviceService;
    SamplerControllerS m_samplerController;
    SynthControllerS m_synthController;
    BassSynthControllerS m_bassSynthController;
    DrumSynthControllerS m_drumSynthController;
    EditorServiceS m_editorService;

    QStringList m_devices;
    int m_revision { 0 };
};

} // namespace noteahead

#endif // DEVICE_RACK_CONTROLLER_HPP
