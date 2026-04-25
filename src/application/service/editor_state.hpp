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

#ifndef EDITOR_STATE_HPP
#define EDITOR_STATE_HPP

#include "../position.hpp"
#include "copy_manager.hpp"
#include <QString>

namespace noteahead {

struct EditorState
{
    Position cursorPosition;

    QString createdDate;
    QString currentPatternTime;
    QString currentTime;
    QString duration;

    quint64 horizontalScrollPosition = 0;
    quint64 songPosition = 0;

    CopyManager copyManager;

    bool isModified = false;
};

} // namespace noteahead

#endif // EDITOR_STATE_HPP
