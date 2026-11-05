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
import Noteahead 1.0

//! The body of a LUFS or dBTP meter's panel element: its bars, on the scale and in the colours of
//! the meter's own dialog. These meters always measure, so the element only reads them.
Item {
    id: root

    required property string sourceId
    //! How many bars the meter has, for the element's width
    property int barCount: 3

    readonly property bool running: root.visible && root.width > 0

    implicitWidth: 46 + root.barCount * 44

    function _update(): void {
        performancePanelController.updateRenderer(root.sourceId, renderer);
    }

    Component.onCompleted: root._update()

    // A meter has to be read to move; it is read only while it is shown, as the dialog's is
    Timer {
        interval: 33
        running: root.running
        repeat: true
        onTriggered: root._update()
    }

    LevelMeterRenderer {
        id: renderer
        anchors.fill: parent
        accentColor: themeService.accentColor
    }
}
