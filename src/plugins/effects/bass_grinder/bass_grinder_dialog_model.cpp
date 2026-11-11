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

#include "bass_grinder_dialog_model.hpp"

#include "bass_grinder.hpp"

namespace noteahead {

BassGrinderDialogModel::BassGrinderDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float BassGrinderDialogModel::saturationDb() const
{
    if (const auto effect = this->effect(); effect) {
        if (const auto bassGrinder = std::dynamic_pointer_cast<BassGrinder>(effect); bassGrinder) {
            return bassGrinder->saturationDb();
        }
    }

    return 0.0f;
}

} // namespace noteahead
