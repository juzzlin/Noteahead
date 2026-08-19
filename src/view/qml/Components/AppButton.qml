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

// Qt's own AbstractButton press/release detection intermittently drops clicks on some Qt 6.10
// installs while the audio engine is running; a plain MouseArea's simpler grab never does, which is
// why ToolBarButtonBase already rolls its own. Drop-in replacement for Button wherever the control
// has to stay clickable during playback -- the overlaid MouseArea sits on top and consumes the
// press before Button's own handling ever sees it, so this never double-fires.
Button {
    id: root
    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
