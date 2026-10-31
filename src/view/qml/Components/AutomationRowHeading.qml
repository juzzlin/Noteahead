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

// The title of an automation in an edit dialog, with its curve drawn after it on the same line: a
// SectionBox label, so the row stays as tall as it was.
Item {
    id: root

    property string text: ""
    //! The automation's curveShape role.
    property var shape: []
    property real line0: 0
    property real line1: 1
    property real minValue: 0
    property real maxValue: 1

    implicitHeight: heading.implicitHeight

    SectionHeading {
        id: heading
        text: root.text
        // The title first: the preview takes what is left of the line
        width: Math.min(implicitWidth, root.width)
        elide: Text.ElideRight
    }

    AutomationCurveRenderer {
        compact: true
        anchors.left: heading.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.verticalCenter: heading.verticalCenter
        height: heading.height
        visible: width > 20
        nodes: root.shape
        line0: root.line0
        line1: root.line1
        minValue: root.minValue
        maxValue: root.maxValue
        accentColor: themeService.accentColor
    }
}
