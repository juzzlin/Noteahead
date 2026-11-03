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

#ifndef PERFORMANCE_PANEL_CONTROLLER_HPP
#define PERFORMANCE_PANEL_CONTROLLER_HPP

#include <QObject>
#include <QPointer>
#include <QVariantList>

#include <map>
#include <memory>

namespace noteahead {

class DeviceService;
class PerformancePanelModel;
class XyPadController;

//! What the Performance Panel's elements point at: finds a device by the id an element keeps, and
//! hands each XY pad element a controller of its own, so that the panel can play every pad device
//! in the rack at once while the XY Pad dialog edits any one of them.
class PerformancePanelController : public QObject
{
    Q_OBJECT

    //! Bumped whenever the rack changes, for bindings that look something up by id to read again.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    //! The element types, as the model's type role holds them.
    Q_PROPERTY(QString xyPadType READ xyPadType CONSTANT)
    //! The device a new XY pad element puts in the rack.
    Q_PROPERTY(QString xyPadDeviceTypeId READ xyPadDeviceTypeId CONSTANT)

public:
    PerformancePanelController(std::shared_ptr<DeviceService> deviceService, PerformancePanelModel & model, QObject * parent = nullptr);

    int revision() const;
    QString xyPadType() const;
    QString xyPadDeviceTypeId() const;

    //! Everything that could be put on the panel now, one map per element: type, sourceId, the
    //! device's slot and whether the panel already shows it.
    Q_INVOKABLE QVariantList availableElements() const;
    //! Puts the device in @p slotIndex on the panel, if the panel can show it.
    Q_INVOKABLE bool addDevice(int slotIndex);
    //! The rack slot of the device with uuid @p sourceId, or -1 when it is gone.
    Q_INVOKABLE int deviceSlot(const QString & sourceId) const;
    //! A controller playing the XY pad device with uuid @p sourceId, or null when there is none.
    //! Owned by this controller and kept for as long as the device is.
    Q_INVOKABLE QObject * xyPadController(const QString & sourceId);

signals:
    void revisionChanged();

private:
    void onDevicesChanged();

    std::shared_ptr<DeviceService> m_deviceService;
    PerformancePanelModel & m_model;
    std::map<QString, QPointer<XyPadController>> m_xyPadControllers;
    int m_revision { 0 };
};

} // namespace noteahead

#endif // PERFORMANCE_PANEL_CONTROLLER_HPP
