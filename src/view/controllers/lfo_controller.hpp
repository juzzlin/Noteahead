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

#ifndef LFO_CONTROLLER_HPP
#define LFO_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QStringList>
#include <QVariantList>

#include <memory>
#include <vector>

namespace noteahead {

class DeviceService;
class LfoDevice;
class LfoPanelController;

//! The LFO device's dialog: four LFO panels, each moving one parameter elsewhere in the project.
class LfoController : public DeviceController
{
    Q_OBJECT

    //! The four panels, in order. Constant: the objects live as long as the controller.
    Q_PROPERTY(QVariantList lfos READ lfos CONSTANT)
    Q_PROPERTY(QStringList waveformNames READ waveformNames CONSTANT)
    Q_PROPERTY(int maxRateDivider READ maxRateDivider CONSTANT)

public:
    explicit LfoController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    Q_INVOKABLE void setDevice(const QString & deviceName);

    QVariantList lfos() const;
    QStringList waveformNames() const;
    int maxRateDivider() const;

    Q_INVOKABLE void requestSettings() override;

    //! The device the panels read and write, or nothing.
    std::shared_ptr<LfoDevice> lfoDevice() const;

private:
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<LfoDevice> m_device;
    std::vector<LfoPanelController *> m_panels;
};

} // namespace noteahead

#endif // LFO_CONTROLLER_HPP
