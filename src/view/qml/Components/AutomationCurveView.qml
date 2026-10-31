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
import ".."

// A drawn automation, edited with the mouse: a press adds a node, or takes hold of the one under it,
// and a drag moves it; a right press deletes it. Nodes land on whole lines unless Shift is held.
// Everything that decides where a node may go is the editor's; this only forwards the mouse.
AutomationCurveRenderer {
    id: root

    //! The AutomationCurveEditor holding the curve.
    property var editor: null
    //! Reads a value out the way the automation's own controls do.
    property var formatValue: value => value.toFixed(2)
    property int dragIndex: -1

    nodes: editor ? editor.nodes : []
    line0: editor ? editor.line0 : 0
    line1: editor ? editor.line1 : 1
    minValue: editor ? editor.minValue : 0
    maxValue: editor ? editor.maxValue : 1
    accentColor: themeService.accentColor

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: root.dragIndex >= 0 || root.hoveredNode >= 0 ? Qt.PointingHandCursor : Qt.CrossCursor
        function snaps(mouse): bool {
            return !(mouse.modifiers & Qt.ShiftModifier);
        }
        onPressed: mouse => {
            root.editor.beginGesture();
            if (mouse.button === Qt.RightButton) {
                root.editor.removeNode(root.nodeAt(mouse.x, mouse.y));
                root.editor.endGesture();
                root.hoveredNode = root.nodeAt(mouse.x, mouse.y);
                return;
            }
            let index = root.nodeAt(mouse.x, mouse.y);
            if (index < 0) {
                index = root.editor.insertNode(root.lineAt(mouse.x), root.valueAt(mouse.y), snaps(mouse));
            }
            root.dragIndex = index;
            root.hoveredNode = index;
        }
        onPositionChanged: mouse => {
            if (root.dragIndex >= 0) {
                root.editor.moveNode(root.dragIndex, root.lineAt(mouse.x), root.valueAt(mouse.y), snaps(mouse));
            } else {
                root.hoveredNode = root.nodeAt(mouse.x, mouse.y);
            }
        }
        onReleased: mouse => {
            if (mouse.button === Qt.LeftButton) {
                root.dragIndex = -1;
                root.editor.endGesture();
            }
        }
        onExited: {
            if (root.dragIndex < 0) {
                root.hoveredNode = -1;
            }
        }
    }

    // Where the node under the mouse is, beside it and kept inside the view
    Rectangle {
        id: readout
        readonly property var node: root.hoveredNode >= 0 && root.hoveredNode < root.nodes.length ? root.nodes[root.hoveredNode] : null
        readonly property real nodeX: node ? root.xForLine(node.line) : 0
        readonly property real nodeY: node ? root.yForValue(node.value) : 0
        visible: node !== null
        width: readoutText.implicitWidth + 10
        height: readoutText.implicitHeight + 4
        x: Math.max(0, Math.min(root.width - width, nodeX + 10))
        y: Math.max(0, Math.min(root.height - height, nodeY - height - 10))
        radius: 2
        color: Theme.surfaceRaised
        opacity: 0.9
        Text {
            id: readoutText
            anchors.centerIn: parent
            text: readout.node ? qsTr("Line %1: %2").arg(Number.isInteger(readout.node.line) ? readout.node.line : readout.node.line.toFixed(2)).arg(root.formatValue(readout.node.value)) : ""
            font.pixelSize: 11
            color: Theme.textBright
        }
    }
}
