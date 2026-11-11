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

#ifndef DYNAMIC_EQ_DIALOG_MODEL_HPP
#define DYNAMIC_EQ_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class DynamicEqDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit DynamicEqDialogModel(QObject * parent = nullptr);

    // DynamicEq
    //! The band at its current gain, and at the whole of its Range: where it is and how far it can go.
    Q_INVOKABLE QVariantList response(int points) const;
    Q_INVOKABLE QVariantList rangeResponse(int points) const;
    Q_INVOKABLE float gainDb() const;
    Q_INVOKABLE float rangeDb() const;
    //! What the detector hears, in dB, to read against the threshold.
    Q_INVOKABLE float detectorLevelDb() const;
    //! The band's gain over the last couple of seconds, oldest first.
    Q_INVOKABLE QVariantList gainHistory() const;
    //! The history is kept only while the dialog is open.
    Q_INVOKABLE void setActive(bool active);
};

} // namespace noteahead

#endif // DYNAMIC_EQ_DIALOG_MODEL_HPP
