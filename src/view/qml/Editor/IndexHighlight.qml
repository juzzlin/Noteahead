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

import QtQuick 2.15

Rectangle {
    property int index: 0
    readonly property int _beatLine1: editorService.linesPerBeat
    readonly property int _beatLine2: _beatLine1 % 3 ? _beatLine1 / 2 : _beatLine1 / 3
    readonly property int _beatLine3: _beatLine1 % 6 ? _beatLine1 / 4 : _beatLine1 / 6
    color: "white"
    opacity: {
        if (!(index % _beatLine1))
            return 0.25;
        if (!(index % _beatLine3) && !(index % _beatLine2))
            return 0.10;
        if (!(index % _beatLine3))
            return 0.05;
        return 0;
    }
    visible: opacity > 0
}
