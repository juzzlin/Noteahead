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
#include <QVariantMap>

#include <map>
#include <memory>

namespace noteahead {

class DeviceService;
class PerformancePanelModel;
class Effect;
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
    Q_PROPERTY(QString rtaType READ rtaType CONSTANT)
    Q_PROPERTY(QString lufsMeterType READ lufsMeterType CONSTANT)
    Q_PROPERTY(QString dbtpMeterType READ dbtpMeterType CONSTANT)
    Q_PROPERTY(QString stereoFieldMeterType READ stereoFieldMeterType CONSTANT)
    //! The device a new XY pad element puts in the rack.
    Q_PROPERTY(QString xyPadDeviceTypeId READ xyPadDeviceTypeId CONSTANT)

public:
    PerformancePanelController(std::shared_ptr<DeviceService> deviceService, PerformancePanelModel & model, QObject * parent = nullptr);

    int revision() const;
    QString xyPadType() const;
    QString rtaType() const;
    QString lufsMeterType() const;
    QString dbtpMeterType() const;
    QString stereoFieldMeterType() const;
    QString xyPadDeviceTypeId() const;

    //! Everything that could be put on the panel now, one map per element: type, sourceId,
    //! whether the panel already shows it, and the device's slot or the effect's location.
    Q_INVOKABLE QVariantList availableElements() const;
    //! Puts the device in @p slotIndex on the panel, if the panel can show it.
    Q_INVOKABLE bool addDevice(int slotIndex);
    //! Puts the effect with id @p effectId on the panel, if the panel can show it.
    Q_INVOKABLE bool addEffect(const QString & effectId);
    //! Whether elements of @p type show an effect rather than a device.
    Q_INVOKABLE bool isEffectType(const QString & type) const;
    //! The rack slot of the device with uuid @p sourceId, or -1 when it is gone.
    Q_INVOKABLE int deviceSlot(const QString & sourceId) const;
    //! A controller playing the XY pad device with uuid @p sourceId, or null when there is none.
    //! Owned by this controller and kept for as long as the device is.
    Q_INVOKABLE QObject * xyPadController(const QString & sourceId);

    //! Where the effect with id @p sourceId sits, as automation targets name it (owner, type,
    //! rack, deviceSlot, deviceTypeName, rackIndex, effectSlot) plus the deviceName its rack is
    //! opened by. Empty when the effect is gone.
    Q_INVOKABLE QVariantMap effectLocation(const QString & sourceId) const;
    //! Starts or stops the analysis of the meter with id @p sourceId for the panel, for the meters
    //! that analyse only while shown. The panel counts as one viewer however often it asks, and
    //! lets go of the same meter it took even after the rack has moved on.
    Q_INVOKABLE void setAnalysisActive(const QString & sourceId, bool active);
    //! Copies the readings of the meter with id @p sourceId, and its display settings, into
    //! @p renderer: an RtaRenderer, a StereoFieldRenderer or a LevelMeterRenderer.
    Q_INVOKABLE void updateRenderer(const QString & sourceId, QObject * renderer) const;

    //! Reads the rack again: drops what points at devices that are gone and bumps the revision.
    void refresh();

signals:
    void revisionChanged();

private:
    std::shared_ptr<DeviceService> m_deviceService;
    PerformancePanelModel & m_model;
    std::map<QString, QPointer<XyPadController>> m_xyPadControllers;
    //! What setAnalysisActive() took, so that it lets go of the same meter
    std::map<QString, std::weak_ptr<Effect>> m_analysedEffects;
    int m_revision { 0 };
};

} // namespace noteahead

#endif // PERFORMANCE_PANEL_CONTROLLER_HPP
