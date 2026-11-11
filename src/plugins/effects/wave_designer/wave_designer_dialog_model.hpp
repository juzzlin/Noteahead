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

#ifndef WAVE_DESIGNER_DIALOG_MODEL_HPP
#define WAVE_DESIGNER_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class WaveDesignerDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit WaveDesignerDialogModel(QObject * parent = nullptr);

    Q_INVOKABLE float shapingDb() const;
};

} // namespace noteahead

#endif // WAVE_DESIGNER_DIALOG_MODEL_HPP
