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

#ifndef LFO_PANEL_CONTROLLER_HPP
#define LFO_PANEL_CONTROLLER_HPP

#include <QObject>
#include <QString>

#include <cstddef>

namespace noteahead {

class LfoController;

//! One LFO of the LFO device, as QML sees it: the dialog repeats one panel over four of these.
//!
//! Continuous controls are knob positions scaled by Constants::uiInternalScaling(); waveform, mode,
//! divider and modulation mode are their plain values. The target is handed over as the three
//! strings a parameter automation names one by, so the panel can use the same target list.
class LfoPanelController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index CONSTANT)

    // One notification for the lot, as on the FM operators
    Q_PROPERTY(int waveform READ waveform WRITE setWaveform NOTIFY changed)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(int rate READ rate WRITE setRate NOTIFY changed)
    Q_PROPERTY(int rateDivider READ rateDivider WRITE setRateDivider NOTIFY changed)
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY changed)
    Q_PROPERTY(int modulationMode READ modulationMode WRITE setModulationMode NOTIFY changed)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY changed)
    Q_PROPERTY(int fade READ fade WRITE setFade NOTIFY changed)
    Q_PROPERTY(int phase READ phase WRITE setPhase NOTIFY changed)
    Q_PROPERTY(int smoothing READ smoothing WRITE setSmoothing NOTIFY changed)
    Q_PROPERTY(bool retrigger READ retrigger WRITE setRetrigger NOTIFY changed)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY changed)

    //! "device" or "effect", the owner's id and the parameter's key; empty when nothing is targeted
    Q_PROPERTY(QString targetOwner READ targetOwner NOTIFY changed)
    Q_PROPERTY(QString targetOwnerId READ targetOwnerId NOTIFY changed)
    Q_PROPERTY(QString targetParameter READ targetParameter NOTIFY changed)

public:
    LfoPanelController(size_t index, LfoController & owner);

    int index() const;

    int waveform() const;
    void setWaveform(int value);
    int mode() const;
    void setMode(int value);
    int rate() const;
    void setRate(int value);
    int rateDivider() const;
    void setRateDivider(int value);
    int depth() const;
    void setDepth(int value);
    int modulationMode() const;
    void setModulationMode(int value);
    int delay() const;
    void setDelay(int value);
    int fade() const;
    void setFade(int value);
    int phase() const;
    void setPhase(int value);
    int smoothing() const;
    void setSmoothing(int value);
    bool retrigger() const;
    void setRetrigger(bool value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);

    QString targetOwner() const;
    QString targetOwnerId() const;
    QString targetParameter() const;
    Q_INVOKABLE void setTarget(const QString & owner, const QString & ownerId, const QString & parameter);
    Q_INVOKABLE void clearTarget();

signals:
    void changed();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    int choice(const QString & key) const;
    void setChoice(const QString & key, int value);

    size_t m_index;
    LfoController & m_owner;
};

} // namespace noteahead

#endif // LFO_PANEL_CONTROLLER_HPP
