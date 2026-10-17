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
import Noteahead 1.0

//! A tab: readable when it is not the current one, and marked with the accent when it is.
TabButton {
    id: control

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.bodyPixelSize + 1
        font.bold: control.checked
        color: control.checked ? Theme.textBright : (control.hovered ? Theme.textSecondary : Theme.textMuted)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: 32
        color: control.checked ? Theme.surfacePanel : "transparent"

        // The accent along the top edge of the current tab, which is all of it the accent touches
        Rectangle {
            width: parent.width
            height: 2
            color: themeService.accentColor
            visible: control.checked
        }
    }
}
