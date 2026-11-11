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

#ifndef XY_PAD_DIALOG_MODEL_HPP
#define XY_PAD_DIALOG_MODEL_HPP

#include "../../../view/controllers/device_dialog_model.hpp"

#include <QVariantList>

#include <memory>
#include <vector>

namespace noteahead {

class DeviceService;
class XyPadDevice;
class XyPadPanelController;

//! The XY pad device, as its settings dialog and the pads themselves see it: four pad panels, plus
//! the hand on the pads, which goes straight to the device's lock-free inputs.
class XyPadDialogModel : public DeviceDialogModel
{
    Q_OBJECT

    //! The four pad panels, in order. Constant: the objects live as long as the controller.
    Q_PROPERTY(QVariantList pads READ pads CONSTANT)
    Q_PROPERTY(int padCount READ padCount CONSTANT)

public:
    explicit XyPadDialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    Q_INVOKABLE void setDevice(const QString & deviceName);

    QVariantList pads() const;
    int padCount() const;

    //! The hand on pad @p pad, at @p x, @p y from -1 to 1 with y up. Lock-free: safe to call on
    //! every mouse move.
    Q_INVOKABLE void press(int pad, double x, double y);
    Q_INVOKABLE void move(int pad, double x, double y);
    Q_INVOKABLE void release(int pad);

    //! Where pad @p pad stands now, as the audio thread last moved it.
    Q_INVOKABLE double positionX(int pad) const;
    Q_INVOKABLE double positionY(int pad) const;
    Q_INVOKABLE bool isPressed(int pad) const;
    //! Whether pad @p pad is still on its way somewhere: what keeps a crosshair redrawing.
    Q_INVOKABLE bool isMoving(int pad) const;

    Q_INVOKABLE void requestSettings() override;

signals:
    //! A hand went on pad @p pad, through this controller or any other view of the same device.
    void padPressed(int pad);
    //! An automation will move the device's pads @p delayMs from now: a recorded take playing.
    void padsAutomated(qint64 delayMs);

public:
    //! The device the panels read and write, or nothing.
    std::shared_ptr<XyPadDevice> xyPadDevice() const;
    const std::shared_ptr<DeviceService> & deviceService() const;

private:
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<XyPadDevice> m_device;
    std::vector<XyPadPanelController *> m_panels;
};

} // namespace noteahead

#endif // XY_PAD_DIALOG_MODEL_HPP
