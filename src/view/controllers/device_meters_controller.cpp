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

#include "device_meters_controller.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <algorithm>

namespace noteahead {

DeviceMetersController::DeviceMetersController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, &DeviceMetersController::updateDevices);
    }
    if (editorService) {
        connect(editorService.get(), &EditorService::songChanged, this, &DeviceMetersController::updateDevices);
    }
}

DeviceMetersController::~DeviceMetersController() = default;

int DeviceMetersController::slotCount() const
{
    return static_cast<int>(Constants::deviceRackSize());
}

void DeviceMetersController::updateDevices()
{
    if (const auto names = m_deviceService ? m_deviceService->internalDeviceNamesQt() : QStringList {}; names != m_devices) {
        m_devices = names;
        applyMetersActive();
    }
}

QVariantList DeviceMetersController::deviceMeterLevels(int slotIndex) const
{
    QVariantList levels;
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        levels.append(device->meter().peakDb());
        levels.append(device->meter().rmsDb());
    }
    return levels;
}

QVariantMap DeviceMetersController::deviceOutputMeters(int slotIndex) const
{
    QVariantMap meters;
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        meters["peakDb"] = device->outputMeter().peakDb();
        meters["rmsDb"] = device->outputMeter().rmsDb();
        meters["shortTermLufs"] = device->outputLoudnessMeter().shortTermLufs();
        meters["integratedLufs"] = device->outputLoudnessMeter().integratedLufs();
    }
    return meters;
}

void DeviceMetersController::resetDeviceLoudness(int slotIndex)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->outputLoudnessMeter().requestReset();
    }
}

void DeviceMetersController::resetAllDeviceLoudness()
{
    for (int slotIndex = 0; slotIndex < slotCount(); slotIndex++) {
        resetDeviceLoudness(slotIndex);
    }
}

void DeviceMetersController::setMetersActive(bool active)
{
    m_meterViewCount = std::max(0, m_meterViewCount + (active ? 1 : -1));
    m_metersActive = m_meterViewCount > 0;
    applyMetersActive();
}

bool DeviceMetersController::deviceClipped(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    return device ? device->clipDetector().clipped() : false;
}

void DeviceMetersController::clearDeviceClip(int slotIndex)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->clipDetector().clear();
    }
}

void DeviceMetersController::clearAllDeviceClips()
{
    for (int slotIndex = 0; slotIndex < slotCount(); slotIndex++) {
        clearDeviceClip(slotIndex);
    }
}

double DeviceMetersController::deviceLoad(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    return device ? static_cast<double>(device->loadMeter().loadPercent()) : 0.0;
}

double DeviceMetersController::totalLoad() const
{
    const auto engine = m_deviceService->audioEngine();
    return engine ? static_cast<double>(engine->loadMeter().loadPercent()) : 0.0;
}

double DeviceMetersController::totalPeakLoad() const
{
    const auto engine = m_deviceService->audioEngine();
    return engine ? static_cast<double>(engine->loadMeter().peakPercent()) : 0.0;
}

int DeviceMetersController::overrunCount() const
{
    const auto engine = m_deviceService->audioEngine();
    return engine ? static_cast<int>(engine->loadMeter().overrunCount()) : 0;
}

void DeviceMetersController::applyMetersActive()
{
    if (!m_deviceService) {
        return;
    }
    for (int slotIndex = 0; slotIndex < slotCount(); slotIndex++) {
        if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
            device->meter().setActive(m_metersActive);
            device->outputMeter().setActive(m_metersActive);
            device->outputLoudnessMeter().setActive(m_metersActive);
            device->loadMeter().setActive(m_metersActive);
        }
    }
    if (const auto engine = m_deviceService->audioEngine()) {
        engine->loadMeter().setActive(m_metersActive);
    }
}

} // namespace noteahead
