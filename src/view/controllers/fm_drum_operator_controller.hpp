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

#ifndef FM_DRUM_OPERATOR_CONTROLLER_HPP
#define FM_DRUM_OPERATOR_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

#include <cstddef>

namespace noteahead {

class FmDrumsController;

//! One operator of the selected FM Drums pad, as QML sees it.
//!
//! The same arrangement as FmOperatorController and for the same reason: one panel repeated over
//! four of these keeps the bindings real bindings. It follows whichever pad is selected, so it reads
//! and writes through the owning controller rather than holding a device of its own.
class FmDrumOperatorController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)

    // One notification for the lot, as on the FM Synth's operators
    Q_PROPERTY(int waveform READ waveform WRITE setWaveform NOTIFY changed)
    Q_PROPERTY(int ratio READ ratio WRITE setRatio NOTIFY changed)
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY changed)
    Q_PROPERTY(int attack READ attack WRITE setAttack NOTIFY changed)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY changed)
    Q_PROPERTY(int curve READ curve WRITE setCurve NOTIFY changed)

    //! Whether the pad's algorithm hears this operator directly: the level knob is loudness on a
    //! carrier and brightness on a modulator, and the panel says which.
    Q_PROPERTY(bool carrier READ carrier NOTIFY changed)
    //! Operator numbers, one based, whose output this one's phase is modulated by.
    Q_PROPERTY(QVariantList modulatedBy READ modulatedBy NOTIFY changed)
    //! Whether the feedback loop is on this operator.
    Q_PROPERTY(bool feedbackOperator READ feedbackOperator CONSTANT)

public:
    FmDrumOperatorController(size_t index, FmDrumsController & owner);

    int index() const;
    QString title() const;

    int waveform() const;
    void setWaveform(int waveform);
    int ratio() const;
    void setRatio(int ratio);
    int level() const;
    void setLevel(int level);
    int attack() const;
    void setAttack(int attack);
    int decay() const;
    void setDecay(int decay);
    int curve() const;
    void setCurve(int curve);

    bool carrier() const;
    QVariantList modulatedBy() const;
    bool feedbackOperator() const;

signals:
    void changed();

private:
    size_t m_index;
    FmDrumsController & m_owner;
};

} // namespace noteahead

#endif // FM_DRUM_OPERATOR_CONTROLLER_HPP
