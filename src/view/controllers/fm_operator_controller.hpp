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

#ifndef FM_OPERATOR_CONTROLLER_HPP
#define FM_OPERATOR_CONTROLLER_HPP

#include <QObject>
#include <QString>

#include <cstddef>
#include <memory>

namespace noteahead {

class FmSynthDevice;

//! One operator of the FM synth, as QML sees it.
//!
//! The four operators are identical, so the dialog draws one panel and repeats it over these
//! rather than carrying four copies of the same forty bindings. Handing QML an object per operator
//! is also what keeps the panel's bindings real bindings: a property read through an index would
//! not re-evaluate when the value behind it moved.
class FmOperatorController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int index READ index CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)

    // One notification for the lot. The panel is ten knobs, so re-reading all of them when any one
    // moves costs nothing worth a signal each.
    Q_PROPERTY(int waveform READ waveform WRITE setWaveform NOTIFY changed)
    Q_PROPERTY(int ratio READ ratio WRITE setRatio NOTIFY changed)
    Q_PROPERTY(QString ratioText READ ratioText NOTIFY changed)
    Q_PROPERTY(int detune READ detune WRITE setDetune NOTIFY changed)
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY changed)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY changed)
    Q_PROPERTY(int keyScale READ keyScale WRITE setKeyScale NOTIFY changed)
    Q_PROPERTY(int attack READ attack WRITE setAttack NOTIFY changed)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY changed)
    Q_PROPERTY(int sustain READ sustain WRITE setSustain NOTIFY changed)

    //! Whether the current algorithm hears this operator directly. The panel says so, because the
    //! same level knob means loudness on a carrier and brightness on a modulator.
    Q_PROPERTY(bool carrier READ carrier NOTIFY routingChanged)
    //! Operator numbers, one based, whose output this one's phase is modulated by.
    Q_PROPERTY(QVariantList modulatedBy READ modulatedBy NOTIFY routingChanged)
    //! Whether the algorithm's feedback loop is on this operator.
    Q_PROPERTY(bool feedbackOperator READ feedbackOperator CONSTANT)

public:
    FmOperatorController(size_t index, QObject * parent = nullptr);

    void setSynth(std::shared_ptr<FmSynthDevice> synth);

    //! Re-reads everything. Called by the owning controller when the device or the patch changed.
    void refresh();
    //! Re-reads only what the algorithm decides. Called when the algorithm changed.
    void refreshRouting();

    int index() const;
    QString title() const;

    int waveform() const;
    void setWaveform(int waveform);
    int ratio() const;
    void setRatio(int ratio);
    QString ratioText() const;
    int detune() const;
    void setDetune(int detune);
    int level() const;
    void setLevel(int level);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int sensitivity);
    int keyScale() const;
    void setKeyScale(int keyScale);
    int attack() const;
    void setAttack(int a);
    int decay() const;
    void setDecay(int d);
    int sustain() const;
    void setSustain(int s);

    bool carrier() const;
    QVariantList modulatedBy() const;
    bool feedbackOperator() const;

signals:
    void changed();
    void routingChanged();

private:
    size_t m_index;
    std::shared_ptr<FmSynthDevice> m_synth;
};

} // namespace noteahead

#endif // FM_OPERATOR_CONTROLLER_HPP
