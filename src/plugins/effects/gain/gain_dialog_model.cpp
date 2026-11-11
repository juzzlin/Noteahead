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

#include "gain_dialog_model.hpp"

#include "gain.hpp"

namespace noteahead {

GainDialogModel::GainDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

bool GainDialogModel::clipped() const
{
    if (const auto effect = this->effect(); effect) {
        if (const auto gain = std::dynamic_pointer_cast<Gain>(effect); gain) {
            return gain->clipDetector().clipped();
        }
    }
    return false;
}

void GainDialogModel::clearClip()
{
    if (const auto effect = this->effect(); effect) {
        if (const auto gain = std::dynamic_pointer_cast<Gain>(effect); gain) {
            gain->clipDetector().clear();
        }
    }
}

} // namespace noteahead
