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

#ifndef PROPERTY_SERVICE_HPP
#define PROPERTY_SERVICE_HPP

#include <QObject>
#include <QVariantList>
#include <memory>
#include <optional>

#include "../../domain/devices/device.hpp"

namespace noteahead {

class DeviceService;

class PropertyService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList availableMidiControllers READ availableMidiControllers CONSTANT)

public:
    explicit PropertyService(QObject * parent = nullptr);

    void setDeviceService(std::weak_ptr<DeviceService> deviceService);

    QVariantList availableMidiControllers() const;
    Q_INVOKABLE QVariantList getAvailableMidiControllers(const QString & portName = {}) const;

    //! Value range a controller accepts on the given port.
    //!
    //! External ports get the MIDI 1.0 range, because anything wider is not a legal data byte on
    //! the wire. An internal device answers for itself through availableMidiCcControllers(), which
    //! is how the fader exposes its boost range. An unknown or empty port falls back to MIDI 1.0.
    Q_INVOKABLE int minValue(int controller, const QString & portName = {}) const;
    Q_INVOKABLE int maxValue(int controller, const QString & portName = {}) const;

    //! Everything a parameter automation can drive: the devices, then every effect in the project,
    //! each with where it sits and the parameters it offers.
    //!
    //! Every entry carries owner ("device" or "effect"), ownerId and parameters. A device adds
    //! deviceSlot and deviceTypeName; an effect adds typeId, type, rack (DeviceService::EffectLocation
    //! ::Rack as an int), deviceSlot, deviceTypeName, rackIndex and effectSlot. Each parameter carries
    //! key, label, unit, mapping, min, max, rest and knob (its authored position) -- what Knob.qml and
    //! KnobController take, so a value reads and is typed exactly as on the knob.
    //!
    //! Only continuous parameters are offered: a position between two choices of a switch means
    //! nothing. An effect offers all of them, as a bare percentage of the travel where it has not
    //! said how one reads out. A device offers only those it has described: it has far more, most of
    //! them the inner workings of a voice or a pad, and listing them all would bury the few worth
    //! riding. A device that describes none is not listed.
    Q_INVOKABLE QVariantList automationTargets() const;
    //! One parameter of automationTargets(), or an empty map when the target or the parameter is gone.
    Q_INVOKABLE QVariantMap automationParameter(const QString & owner, const QString & ownerId, const QString & key) const;

private:
    //! The controller as the port's device declares it, or nothing when the port is not an
    //! internal device or does not offer that controller.
    std::optional<MidiCcController> deviceController(int controller, const QString & portName) const;

    std::weak_ptr<DeviceService> m_deviceService;
};

} // namespace noteahead

#endif // PROPERTY_SERVICE_HPP
