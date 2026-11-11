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

#include "stereo_exciter_dialog_model.hpp"

#include "stereo_exciter.hpp"

namespace noteahead {

StereoExciterDialogModel::StereoExciterDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float StereoExciterDialogModel::harmonicsDb() const
{
    if (const auto effect = this->effect(); effect) {
        if (const auto exciter = std::dynamic_pointer_cast<StereoExciter>(effect); exciter) {
            return exciter->harmonicsDb();
        }
    }

    return 0.0f;
}

} // namespace noteahead
