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

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "bass_synth_controller.hpp"
#include "drum_synth_controller.hpp"
#include "piano_synth_controller.hpp"
#include "sampler_controller.hpp"
#include "synth_controller.hpp"

#include <QVariantMap>

namespace noteahead {

DeviceRackController::DeviceRackController(DeviceServiceS deviceService, ControllerList controllers, EditorServiceS editorService, QObject * parent)
  : QAbstractListModel { parent }
  , m_deviceService { std::move(deviceService) }
  , m_controllers { std::move(controllers) }
  , m_editorService { std::move(editorService) }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, [this]() {
            m_revision++;
            emit revisionChanged();
            refresh();
        });
    }
    if (m_editorService) {
        connect(m_editorService.get(), &EditorService::songChanged, this, &DeviceRackController::refresh);
    }
    refresh();
}

DeviceRackController::~DeviceRackController() = default;

int DeviceRackController::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return deviceCount();
}

QVariant DeviceRackController::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || index.row() >= deviceCount()) {
        return {};
    }

    const auto slotIndex = static_cast<size_t>(index.row());
    const auto dev = m_deviceService->device(slotIndex);

    switch (static_cast<DataRole>(role)) {
    case DataRole::Name:
        return dev ? QString::fromStdString(dev->name()) : QString {};
    case DataRole::TrackNames:
        return dev ? trackNames(static_cast<int>(slotIndex)) : QString {};
    case DataRole::TypeName:
        return dev ? QString::fromStdString(dev->typeName()) : QString {};
    case DataRole::TypeId:
        return dev ? QString::fromStdString(dev->typeId()) : QString {};
    }
    return {};
}

QHash<int, QByteArray> DeviceRackController::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { static_cast<int>(DataRole::Name), "name" },
        { static_cast<int>(DataRole::TrackNames), "trackNames" },
        { static_cast<int>(DataRole::TypeName), "typeName" },
        { static_cast<int>(DataRole::TypeId), "typeId" }
    };
    return roles;
}

int DeviceRackController::deviceCount() const
{
    return static_cast<int>(Constants::deviceRackSize());
}

int DeviceRackController::revision() const
{
    return m_revision;
}

void DeviceRackController::refresh()
{
    m_revision++;
    emit revisionChanged();
    beginResetModel();
    if (m_deviceService) {
        m_devices = m_deviceService->internalDeviceNamesQt();
    }
    endResetModel();
}

void DeviceRackController::openDevice(const QString & name)
{
    if (const auto device = m_deviceService->device(name.toStdString()); device) {
        for (const auto & controller : m_controllers) {
            if (controller->setDevice(device)) {
                if (const auto typeId = device->typeId(); typeId == SamplerDevice::typeIdString()) {
                    emit samplerDialogRequested();
                } else if (typeId == SynthDevice::typeIdString()) {
                    emit synthDialogRequested();
                } else if (typeId == WavetableSynthDevice::typeIdString()) {
                    emit wavetableSynthDialogRequested();
                } else if (typeId == BassSynthDevice::typeIdString()) {
                    emit bassSynthDialogRequested();
                } else if (typeId == DrumSynthDevice::typeIdString()) {
                    emit drumSynthDialogRequested();
                } else if (typeId == PianoSynthDevice::typeIdString()) {
                    emit pianoSynthDialogRequested();
                }
                return;
            }
        }
    }
}

void DeviceRackController::openDevice(int slotIndex)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        openDevice(QString::fromStdString(device->name()));
    }
}

void DeviceRackController::requestEffectSendsDialog(const QString & deviceName)
{
    emit effectSendsDialogRequested(deviceName);
}

void DeviceRackController::setDevice(int slotIndex, const QString & typeId)
{
    const auto name = Constants::internalDevicePortPrefix().toStdString() + " " + std::to_string(slotIndex + 1);
    if (const auto device = DeviceFactory::createDevice(typeId.toStdString(), name); device) {
        m_deviceService->setDevice(static_cast<size_t>(slotIndex), std::move(device));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void DeviceRackController::clearDevice(int slotIndex)
{
    m_deviceService->clearDevice(static_cast<size_t>(slotIndex));
    m_editorService->setIsModified(true);
    m_revision++;
    emit revisionChanged();
}

void DeviceRackController::exportSettings(int index, const QUrl & fileUrl)
{
    auto filePath = fileUrl.toLocalFile();
    if (!filePath.endsWith(Constants::deviceSettingsExtension())) {
        filePath += Constants::deviceSettingsExtension();
    }
    m_deviceService->exportDeviceSettings(index, filePath);
}

void DeviceRackController::importSettings(int index, const QUrl & fileUrl)
{
    const auto fileInfo = m_deviceService->peekDeviceTypeInfo(fileUrl.toLocalFile());
    const auto currentDev = m_deviceService->device(static_cast<size_t>(index));
    const auto currentTypeName = currentDev ? QString::fromStdString(currentDev->typeName()) : QString {};
    const auto currentTypeId = currentDev ? QString::fromStdString(currentDev->typeId()) : QString {};
    const bool typeMismatch = currentDev && !fileInfo.typeId.isEmpty() && currentTypeId != fileInfo.typeId;
    emit importSettingsConfirmationRequested(index, fileUrl, currentTypeName, fileInfo.typeName, typeMismatch);
}

void DeviceRackController::confirmImportSettings(int index, const QUrl & fileUrl)
{
    if (m_deviceService->importDeviceSettings(index, fileUrl.toLocalFile())) {
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

QString DeviceRackController::deviceType(int slotIndex) const
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        return QString::fromStdString(device->typeId());
    }
    return "";
}

QString DeviceRackController::deviceTypeName(int slotIndex) const
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        return QString::fromStdString(device->typeName());
    }
    return "";
}

QString DeviceRackController::deviceName(int slotIndex) const
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        return QString::fromStdString(device->name());
    }
    return "";
}

QString DeviceRackController::trackNames(int slotIndex) const
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        const auto deviceName = QString::fromStdString(device->name());
        QStringList trackNames;
        for (const auto index : m_editorService->trackIndices()) {
            if (const auto portName = m_editorService->instrumentPortName(index); portName == deviceName) {
                trackNames << m_editorService->trackName(index);
            }
        }
        return trackNames.join(", ");
    }
    return "";
}

QVariantList DeviceRackController::availableDevices() const
{
    QVariantList list;

    const auto addDevice = [&](const QString & name, const QString & typeId) {
        QVariantMap map;
        map["name"] = name;
        map["typeId"] = typeId;
        list.append(map);
    };

    addDevice("Sampler", QString::fromStdString(SamplerDevice::typeIdString()));
    addDevice("Synth", QString::fromStdString(SynthDevice::typeIdString()));
    addDevice("Wavetable Synth", QString::fromStdString(WavetableSynthDevice::typeIdString()));
    addDevice("Bass Synth", QString::fromStdString(BassSynthDevice::typeIdString()));
    addDevice("Drum Synth", QString::fromStdString(DrumSynthDevice::typeIdString()));
    addDevice("Piano Synth", QString::fromStdString(PianoSynthDevice::typeIdString()));

    return list;
}

void DeviceRackController::addSampler()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(SamplerDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::addSynth()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(SynthDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::addWavetableSynth()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(WavetableSynthDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::addBassSynth()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(BassSynthDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::addDrumSynth()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(DrumSynthDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::addPianoSynth()
{
    for (int i = 0; i < deviceCount(); i++) {
        if (!m_deviceService->device(static_cast<size_t>(i))) {
            setDevice(i, QString::fromStdString(PianoSynthDevice::typeIdString()));
            return;
        }
    }
}

void DeviceRackController::removeDevice(const QString & name)
{
    if (const auto prefix = Constants::internalDevicePortPrefix(); name.startsWith(prefix)) {
        const auto slotStr = name.mid(prefix.length() + 1);
        const auto slotIndex = slotStr.toUInt() - 1;
        clearDevice(static_cast<int>(slotIndex));
    }
}

} // namespace noteahead
