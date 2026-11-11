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

#include "tube_stage_dialog_model.hpp"

#include "tube_stage.hpp"

namespace noteahead {

TubeStageDialogModel::TubeStageDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float TubeStageDialogModel::saturationDb() const
{
    if (const auto effect = this->effect(); effect) {
        if (const auto tubeStage = std::dynamic_pointer_cast<TubeStage>(effect); tubeStage) {
            return tubeStage->saturationDb();
        }
    }

    return 0.0f;
}

} // namespace noteahead
