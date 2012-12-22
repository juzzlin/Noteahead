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

#ifndef SEND_CONTROLLER_HPP
#define SEND_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace noteahead {

class DeviceService;
class EditorService;

//! How much of a device, or of one part of it, goes to each master send bus, and what the buses
//! hold.
class SendController : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    SendController(DeviceServiceS deviceService, QObject * parent = nullptr);
    ~SendController() override;

    Q_INVOKABLE float deviceSend(const QString & deviceName, quint32 effectIndex) const;
    Q_INVOKABLE void setDeviceSend(const QString & deviceName, quint32 effectIndex, float send);
    //! The same, for one part of a device: a Sampler pad or a Drum Synth voice.
    //!
    //! @p subIndex below zero means the device itself, so a caller that does not know whether it is
    //! looking at a part can ask the same way either way. A part's send is its own rather than a
    //! share of the device's: both reach the bus, which is what makes a dry kit with one wet drum
    //! possible at all.
    Q_INVOKABLE float partSend(const QString & deviceName, int subIndex, quint32 effectIndex) const;
    Q_INVOKABLE void setPartSend(const QString & deviceName, int subIndex, quint32 effectIndex, float send);
    //! Whether this device has parts that can be routed on their own, so a dialog knows whether to
    //! offer the tab at all.
    Q_INVOKABLE bool hasPartSends(const QString & deviceName) const;
    //! The master send rack, read without pointing the controller at it: a device's Sends tab shows
    //! what the buses are while the controller is still targeting that device's own rack.
    Q_INVOKABLE int masterSendCount() const;
    Q_INVOKABLE QString masterSendEffectType(quint32 busIndex) const;
    Q_INVOKABLE bool hasMasterSendEffects() const;

signals:

private:
    DeviceServiceS m_deviceService;
};

} // namespace noteahead

#endif // SEND_CONTROLLER_HPP
