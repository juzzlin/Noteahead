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
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

//! A menu as wide as its widest row. A Universal menu is a fixed 200 wide, which cuts a long row
//! and runs a label into its shortcut once a check mark takes room from it.
Menu {
    width: {
        let result = 200;
        for (let i = 0; i < count; i++) {
            const item = itemAt(i);
            if (item && item.contentItem) {
                result = Math.max(result, item.contentItem.implicitWidth + item.leftPadding + item.rightPadding);
            }
        }
        return result;
    }
}
