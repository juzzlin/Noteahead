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

// Qt's own AbstractButton press/release detection intermittently drops clicks on some Qt 6.10
// installs while the audio engine is running; a plain MouseArea's simpler grab never does, which is
// why ToolBarButtonBase already rolls its own. Drop-in replacement for Button wherever the control
// has to stay clickable during playback -- the overlaid MouseArea sits on top and consumes the
// press before Button's own handling ever sees it, so this never double-fires.
Button {
    id: root
    // Since the overlay eats the press before Button's own handling ever sees it, `pressed` (and so
    // the default `down`) never goes true on its own -- rebinding `down` to the overlay's own pressed
    // state is what gets the style's normal pressed-color feedback back. `hovered` has no equivalent
    // override (it is read-only), so the background below reads the overlay's containsMouse directly
    // instead of relying on the Control's own -- apparently also overlay-blocked -- hover tracking.
    down: mouseArea.pressed
    // The button that commits a dialog carries a thin accent outline, so the one to press is
    // findable at a glance. Taken from the button's own role rather than set per dialog: every
    // dialog in the application already declares which of its buttons accepts, and there are
    // forty-odd of them.
    //
    // Safe on a button that is not in a DialogButtonBox: the attached role defaults to InvalidRole.
    readonly property bool acceptsDialog: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole
    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32
        visible: !root.flat || root.down || root.checked || root.highlighted
        color: root.down ? root.Universal.baseMediumLowColor : root.enabled && (root.highlighted || root.checked) ? root.Universal.accent : root.Universal.baseLowColor
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: root.enabled && root.acceptsDialog && !root.down
            border.width: 1
            border.color: root.Universal.accent
        }
        // Drawn after the accent outline so hovering still reads as hovering.
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: root.enabled && mouseArea.containsMouse
            border.width: 2
            border.color: root.Universal.baseMediumLowColor
        }
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
