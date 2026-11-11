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

#ifndef NOISE_FX_SWEEP_CONTROLLER_HPP
#define NOISE_FX_SWEEP_CONTROLLER_HPP

#include <QObject>

#include <cstddef>

namespace noteahead {

class NoiseFxController;

//! One sweep of Noise FX, as QML sees it: the dialog shows one panel per sweep over these.
//!
//! Continuous controls are knob positions scaled by Constants::uiInternalScaling(); the mode, the
//! synced length and the targets are their ordinals.
class NoiseFxSweepController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index CONSTANT)

    // One notification for the lot, as on the LFO device's panels
    Q_PROPERTY(int start READ start WRITE setStart NOTIFY changed)
    Q_PROPERTY(int end READ end WRITE setEnd NOTIFY changed)
    Q_PROPERTY(int curve READ curve WRITE setCurve NOTIFY changed)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY changed)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(bool sync READ sync WRITE setSync NOTIFY changed)
    Q_PROPERTY(int syncLength READ syncLength WRITE setSyncLength NOTIFY changed)
    Q_PROPERTY(int target1 READ target1 WRITE setTarget1 NOTIFY changed)
    Q_PROPERTY(int depth1 READ depth1 WRITE setDepth1 NOTIFY changed)
    Q_PROPERTY(int target2 READ target2 WRITE setTarget2 NOTIFY changed)
    Q_PROPERTY(int depth2 READ depth2 WRITE setDepth2 NOTIFY changed)

public:
    NoiseFxSweepController(size_t index, NoiseFxController & owner);

    int index() const;

    int start() const;
    void setStart(int value);
    int end() const;
    void setEnd(int value);
    int curve() const;
    void setCurve(int value);
    int length() const;
    void setLength(int value);
    int mode() const;
    void setMode(int value);
    bool sync() const;
    void setSync(bool value);
    int syncLength() const;
    void setSyncLength(int value);
    int target1() const;
    void setTarget1(int value);
    int depth1() const;
    void setDepth1(int value);
    int target2() const;
    void setTarget2(int value);
    int depth2() const;
    void setDepth2(int value);

signals:
    void changed();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    int choice(const QString & key) const;
    void setChoice(const QString & key, int value);
    int target(size_t destination) const;
    void setTarget(size_t destination, int value);
    int depth(size_t destination) const;
    void setDepth(size_t destination, int value);

    size_t m_index;
    NoiseFxController & m_owner;
};

} // namespace noteahead

#endif // NOISE_FX_SWEEP_CONTROLLER_HPP
