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

#include "reference_dialog_model.hpp"

#include "reference.hpp"

namespace noteahead {

ReferenceDialogModel::ReferenceDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QStringList ReferenceDialogModel::environmentNames() const
{
    // Read from the table the effect itself selects with, so the list cannot name a system the
    // effect does not have or miss one it does.
    QStringList names;
    for (auto && environment : referenceEnvironments()) {
        names << QString::fromStdString(environment.name);
    }
    return names;
}

} // namespace noteahead
