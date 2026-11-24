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

#ifndef GAIN_STAGING_SERVICE_HPP
#define GAIN_STAGING_SERVICE_HPP

#include <QObject>
#include <QTimer>

#include <memory>
#include <optional>

namespace noteahead {

class DeviceService;
class SettingsService;

//! Gain Stage: while its button is held, moves one device's Gain towards the gain staging target.
//!
//! Each step covers a share of the distance, gain += (target + trim - level) * speed, where level is
//! the loudest reading of the session so far, so the gain settles on the loudest hit played rather
//! than creeping up through every gap. The reading is taken before Gain, so what was measured early
//! in the session still holds after Gain has moved. Nothing moves until the device has played.
class GainStagingService : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using SettingsServiceS = std::shared_ptr<SettingsService>;

    GainStagingService(DeviceServiceS deviceService, SettingsServiceS settingsService, QObject * parent = nullptr);
    ~GainStagingService() override;

    //! Starts a session on the device in @p slotIndex, ending any other one first.
    void start(size_t slotIndex);
    //! Ends the session. Returns whether it moved Gain at all.
    bool stop();

    std::optional<size_t> activeSlot() const;

    //! One step of the session; the timer calls it, and tests call it directly.
    void step();

    //! Readings this far under full scale are taken as silence.
    static constexpr float SilenceDb { -80.0f };
    //! The range of the Gain knob, in dB either way.
    static constexpr double GainRangeDb { 30.0 };

signals:
    void activeSlotChanged();

private:
    DeviceServiceS m_deviceService;
    SettingsServiceS m_settingsService;
    QTimer m_timer;

    std::optional<size_t> m_activeSlot;
    //! The loudest reading of the session, before Gain
    std::optional<float> m_sourceMaximumDb;
    bool m_gainMoved { false };
};

} // namespace noteahead

#endif // GAIN_STAGING_SERVICE_HPP
