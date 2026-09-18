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
import QtQuick.Layouts 1.15
import Noteahead 1.0

// Loudness of a device's output, in LUFS. A number rather than a bar on purpose: a bar is read by
// where it reaches, which is the one thing that does not survive a comparison between two different
// sources, and two devices are compared here by subtracting one reading from the other.
//
// The integrated reading leads because it is the one that holds still. Short-term follows it in
// small type, only so that something visibly moves while the song plays.
Item {
    id: root

    //! Floor of the measurement. Nothing has been measured while a reading sits here.
    readonly property real minimumLufs: -70

    property real shortTermLufs: minimumLufs
    property real integratedLufs: minimumLufs

    readonly property bool hasReading: root.integratedLufs > root.minimumLufs

    implicitWidth: 84
    implicitHeight: column.implicitHeight

    function formatLufs(value) {
        return value > root.minimumLufs ? value.toFixed(1) : "–";
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        RowLayout {
            spacing: 3
            Layout.fillWidth: true

            Text {
                text: root.formatLufs(root.integratedLufs)
                color: root.hasReading ? "white" : "#666"
                font.pointSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }

            Text {
                // Not translated: a unit is a unit in every localised DAW there is.
                text: "LUFS"
                color: "#888"
                font.pointSize: 8
            }
        }

        Text {
            // "S" is the standard short-term label on a loudness meter, and stays put untranslated
            // for the same reason the unit does.
            text: "S " + root.formatLufs(root.shortTermLufs)
            color: "#999"
            font.pointSize: 9
            horizontalAlignment: Text.AlignRight
            Layout.fillWidth: true
        }
    }

    ToolTip.visible: hoverArea.containsMouse
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.text: qsTr("Output loudness of this device: %1 LUFS integrated since the mixer was opened or reset, %2 LUFS short-term.").arg(root.formatLufs(root.integratedLufs)).arg(root.formatLufs(root.shortTermLufs)) + "\n" + qsTr("Two devices are balanced by the difference between their integrated readings. Silence is gated out, so this is how loud the device is when it plays, not how often it plays.") + "\n" + qsTr("Measured after the insert effects, the fader and the pan. What the device sends to a send effect returns on the send bus and is not counted here, and a device inside a SubMixer is measured before the SubMixer's own strip.")

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
