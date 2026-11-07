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

#ifndef XY_PAD_PANEL_CONTROLLER_HPP
#define XY_PAD_PANEL_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QStringList>

#include <cstddef>

namespace noteahead {

class XyPadController;

//! One pad of the XY pad device, as its settings dialog sees it: what each axis moves and how far,
//! and how the pad comes back. Continuous controls are knob positions scaled by
//! Constants::uiInternalScaling(). A target is handed over as the three strings a parameter
//! automation names one by, so the dialog can use the same target list. Axis 0 is X, 1 is Y.
class XyPadPanelController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index CONSTANT)

    // One notification for the lot, as on the LFO panels
    Q_PROPERTY(int returnTime READ returnTime WRITE setReturnTime NOTIFY changed)
    Q_PROPERTY(int xDepth READ xDepth WRITE setXDepth NOTIFY changed)
    Q_PROPERTY(int yDepth READ yDepth WRITE setYDepth NOTIFY changed)

    //! "device" or "effect", the owner's id and the parameter's key; empty when nothing is targeted
    Q_PROPERTY(QString xTargetOwner READ xTargetOwner NOTIFY changed)
    Q_PROPERTY(QString xTargetOwnerId READ xTargetOwnerId NOTIFY changed)
    Q_PROPERTY(QString xTargetParameter READ xTargetParameter NOTIFY changed)
    Q_PROPERTY(QString yTargetOwner READ yTargetOwner NOTIFY changed)
    Q_PROPERTY(QString yTargetOwnerId READ yTargetOwnerId NOTIFY changed)
    Q_PROPERTY(QString yTargetParameter READ yTargetParameter NOTIFY changed)
    //! Whether another modulation source -- an LFO, another axis -- moves the same parameter
    Q_PROPERTY(bool xTargetShared READ xTargetShared NOTIFY changed)
    Q_PROPERTY(bool yTargetShared READ yTargetShared NOTIFY changed)

public:
    XyPadPanelController(size_t index, XyPadController & owner);

    int index() const;

    int returnTime() const;
    void setReturnTime(int value);
    int xDepth() const;
    void setXDepth(int value);
    int yDepth() const;
    void setYDepth(int value);

    QString xTargetOwner() const;
    QString xTargetOwnerId() const;
    QString xTargetParameter() const;
    QString yTargetOwner() const;
    QString yTargetOwnerId() const;
    QString yTargetParameter() const;
    bool xTargetShared() const;
    bool yTargetShared() const;

    Q_INVOKABLE void setTarget(int axis, const QString & owner, const QString & ownerId, const QString & parameter);
    Q_INVOKABLE void clearTarget(int axis);
    //! Aims @p axis at @p owner, at the first of @p parameters no modulation source moves yet, and
    //! the other axis, when it has no target, at the next. See DeviceService::suggestedModulationParameters().
    Q_INVOKABLE void chooseTargetOwner(int axis, const QString & owner, const QString & ownerId, const QStringList & parameters);

signals:
    void changed();

private:
    QString targetOwner(int axis) const;
    QString targetOwnerId(int axis) const;
    QString targetParameter(int axis) const;
    bool targetShared(int axis) const;
    int depth(int axis) const;
    void setDepth(int axis, int value);

    size_t m_index;
    XyPadController & m_owner;
};

} // namespace noteahead

#endif // XY_PAD_PANEL_CONTROLLER_HPP
