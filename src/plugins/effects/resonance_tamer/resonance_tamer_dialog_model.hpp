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

#ifndef RESONANCE_TAMER_DIALOG_MODEL_HPP
#define RESONANCE_TAMER_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantList>

namespace noteahead {

class ResonanceTamerDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit ResonanceTamerDialogModel(QObject * parent = nullptr);

    //! What the bells of @p path are taking out now, at @p points frequencies across the curve view:
    //! path 0 is the linked, left or mid one, path 1 the right or side one.
    Q_INVOKABLE QVariantList reduction(int path, int points) const;
    //! The deepest cut now, in dB, negative.
    Q_INVOKABLE float deepestCutDb() const;
};

} // namespace noteahead

#endif // RESONANCE_TAMER_DIALOG_MODEL_HPP
