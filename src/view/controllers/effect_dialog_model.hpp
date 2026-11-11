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

#ifndef EFFECT_DIALOG_MODEL_HPP
#define EFFECT_DIALOG_MODEL_HPP

#include <QObject>
#include <QPointer>
#include <QVariantList>

#include <functional>
#include <memory>
#include <vector>

namespace noteahead {

class Effect;
class EffectRackController;

//! What one effect's dialog needs beyond its parameters: response curves, meter readings and the
//! actions that touch more than one parameter at a time. Each effect that has any subclasses this
//! in its plugin folder, and its dialog creates one for the slot it shows, which keeps all of that
//! out of EffectRackController.
//!
//! The effect is looked up through the rack controller every time rather than held, because the
//! slot can be cleared or refilled while the dialog is open, and which rack is shown is the rack
//! controller's to say.
class EffectDialogModel : public QObject
{
    Q_OBJECT
    //! The EffectRackController whose current rack holds the effect. A QObject, because QML drops
    //! calls with a pointer to a type it only knows by forward declaration.
    Q_PROPERTY(QObject * rack READ rack WRITE setRack NOTIFY rackChanged)
    Q_PROPERTY(int effectIndex READ effectIndex WRITE setEffectIndex NOTIFY effectIndexChanged)

public:
    explicit EffectDialogModel(QObject * parent = nullptr);
    ~EffectDialogModel() override;

    QObject * rack() const;
    void setRack(QObject * rack);

    int effectIndex() const;
    void setEffectIndex(int effectIndex);

    //! Log-spaced from 20 Hz to 20 kHz, the frequencies a response curve is drawn at.
    static std::vector<double> responseFrequencies(int points);
    //! A response curve of @p points values in dB, one at each responseFrequencies() frequency.
    static QVariantList sampleResponse(int points, const std::function<double(double)> & magnitudeDbAt);

signals:
    void rackChanged();
    void effectIndexChanged();

protected:
    //! The effect in the slot now, or nothing when the slot is empty or there is no rack.
    std::shared_ptr<Effect> effect() const;

    float parameterValue(const QString & key) const;
    void setParameterValue(const QString & key, float value);

private:
    QPointer<EffectRackController> m_rack;
    int m_effectIndex = 0;
};

} // namespace noteahead

#endif // EFFECT_DIALOG_MODEL_HPP
