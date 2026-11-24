// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "gain_staging_service.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "device_service.hpp"
#include "settings_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

GainStagingService::GainStagingService(DeviceServiceS deviceService, SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_settingsService { std::move(settingsService) }
{
    m_timer.setInterval(Constants::gainStagingIntervalMs());
    connect(&m_timer, &QTimer::timeout, this, &GainStagingService::step);
}

GainStagingService::~GainStagingService() = default;

void GainStagingService::start(size_t slotIndex)
{
    stop();
    const auto device = m_deviceService->device(slotIndex);
    if (!device) {
        return;
    }
    // Whatever the readings hold was played before the button went down
    device->meter().restartSource();
    m_activeSlot = slotIndex;
    m_sourceMaximumDb.reset();
    m_gainMoved = false;
    m_timer.start();
    emit activeSlotChanged();
}

bool GainStagingService::stop()
{
    const bool gainMoved = m_gainMoved;
    m_timer.stop();
    m_sourceMaximumDb.reset();
    m_gainMoved = false;
    if (m_activeSlot) {
        m_activeSlot.reset();
        emit activeSlotChanged();
    }
    return gainMoved;
}

std::optional<size_t> GainStagingService::activeSlot() const
{
    return m_activeSlot;
}

void GainStagingService::step()
{
    if (!m_activeSlot) {
        return;
    }
    const auto device = m_deviceService->device(*m_activeSlot);
    if (!device) {
        stop();
        return;
    }

    auto & meter = device->meter();
    const float sourcePeakDb = meter.takeSourcePeakDb();
    const float sourceRmsDb = meter.takeSourceRmsDb();
    const float readingDb = device->gainStagingMode() == Device::GainStagingMode::Peak ? sourcePeakDb : sourceRmsDb;
    if (readingDb > SilenceDb) {
        m_sourceMaximumDb = std::max(m_sourceMaximumDb.value_or(readingDb), readingDb);
    }
    if (!m_sourceMaximumDb) {
        return;
    }

    const double gainDb = (static_cast<double>(device->gain()) - 0.5) * 2.0 * GainRangeDb;
    const double targetDb = static_cast<double>(m_settingsService->gainStagingTargetDb()) + device->gainStagingTrimDb();
    const double speed = std::clamp(m_settingsService->gainStagingSpeedPercent(), 1, 100) / 100.0;
    const double levelDb = static_cast<double>(*m_sourceMaximumDb) + gainDb;
    const double newGainDb = std::clamp(gainDb + (targetDb - levelDb) * speed, -GainRangeDb, GainRangeDb);
    if (std::abs(newGainDb - gainDb) > 1.0e-3) {
        device->setGain(static_cast<float>(newGainDb / (2.0 * GainRangeDb) + 0.5));
        m_gainMoved = true;
    }
}

} // namespace noteahead
