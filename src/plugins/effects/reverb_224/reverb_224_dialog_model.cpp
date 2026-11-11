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

#include "reverb_224_dialog_model.hpp"

#include "reverb_224.hpp"

namespace noteahead {

Reverb224DialogModel::Reverb224DialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

bool Reverb224DialogModel::hasEarlyReflections(int program) const
{
    return Reverb224::hasEarlyReflections(static_cast<Reverb224::Program>(std::clamp(program, 0, static_cast<int>(Reverb224::ProgramCount) - 1)));
}

} // namespace noteahead
