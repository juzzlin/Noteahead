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

#ifndef EFFECT_RACK_CONTROLLER_HPP
#define EFFECT_RACK_CONTROLLER_HPP

#include <QObject>
#include <memory>
#include "device_service.hpp"

namespace noteahead {

class EffectRackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int effectCount READ effectCount NOTIFY effectCountChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    explicit EffectRackController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    int effectCount() const;
    int revision() const;

    Q_INVOKABLE float parameterValue(int effectIndex, const QString & paramName) const;
    Q_INVOKABLE void setParameterValue(int effectIndex, const QString & paramName, float value);

    Q_INVOKABLE QString effectType(int effectIndex) const;

    Q_INVOKABLE QStringList reverbPresets() const;
    Q_INVOKABLE void applyReverbPreset(int effectIndex, int presetIndex);

    Q_INVOKABLE float deviceSend(const QString & deviceName, int effectIndex) const;
    Q_INVOKABLE void setDeviceSend(const QString & deviceName, int effectIndex, float send);

signals:
    void effectCountChanged();
    void revisionChanged();
    void parameterChanged(int effectIndex, const QString & paramName);

private:
    std::shared_ptr<DeviceService> m_deviceService;
    int m_revision = 0;
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_HPP
