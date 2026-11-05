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

//! The body of a Stereo Field Meter's panel element: its goniometer and readings, drawn as the
//! meter's own dialog draws them and with the dialog's zoom and guides. The analysis runs only
//! while the element is on screen.
Item {
    id: root

    required property string sourceId

    readonly property bool running: root.visible && root.width > 0

    // About square, and wide enough for the correlation bar under the dial
    implicitWidth: Math.max(root.height, 220)

    function _setActive(active: bool): void {
        performancePanelController.setAnalysisActive(root.sourceId, active);
        if (active) {
            performancePanelController.updateRenderer(root.sourceId, renderer);
        }
    }

    onRunningChanged: root._setActive(root.running)
    Component.onCompleted: root._setActive(root.running)
    Component.onDestruction: performancePanelController.setAnalysisActive(root.sourceId, false)

    Timer {
        interval: 33
        running: root.running
        repeat: true
        onTriggered: performancePanelController.updateRenderer(root.sourceId, renderer)
    }

    StereoFieldRenderer {
        id: renderer
        anchors.fill: parent
        accentColor: themeService.accentColor
        compact: true
    }
}
