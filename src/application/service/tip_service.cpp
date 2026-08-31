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

#include "tip_service.hpp"

namespace noteahead {

TipService::TipService(QObject * parent)
  : QObject { parent }
{
}

QString TipService::currentTip() const
{
    // Kept short on purpose: the bar gives this half of a 1024 px window, and a tip that has to be
    // elided to fit has already failed at being a tip.
    if (m_editMode) {
        return tr("<b>Z–M</b> and <b>Q–U</b> play notes, <b>F3</b>/<b>F4</b> change octave");
    }
    return tr("Press <b>ESC</b> to edit, <b>SPACE</b> to play");
}

void TipService::setEditMode(bool editMode)
{
    if (m_editMode != editMode) {
        m_editMode = editMode;
        emit currentTipChanged();
    }
}

void TipService::retranslate()
{
    emit currentTipChanged();
}

} // namespace noteahead
