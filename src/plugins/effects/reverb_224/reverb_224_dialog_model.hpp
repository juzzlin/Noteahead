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

#ifndef REVERB_224_DIALOG_MODEL_HPP
#define REVERB_224_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class Reverb224DialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit Reverb224DialogModel(QObject * parent = nullptr);

    // Reverb 224
    //! Whether @p program has early reflections for Depth to balance, which the plates do not.
    Q_INVOKABLE bool hasEarlyReflections(int program) const;
};

} // namespace noteahead

#endif // REVERB_224_DIALOG_MODEL_HPP
