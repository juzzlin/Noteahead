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

#ifndef GAIN_DIALOG_MODEL_HPP
#define GAIN_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class GainDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit GainDialogModel(QObject * parent = nullptr);

    //! Whether the Gain in this slot has seen a full-scale sample since it was last cleared.
    Q_INVOKABLE bool clipped() const;
    Q_INVOKABLE void clearClip();
};

} // namespace noteahead

#endif // GAIN_DIALOG_MODEL_HPP
