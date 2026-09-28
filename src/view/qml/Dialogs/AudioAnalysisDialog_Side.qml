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
import QtQuick.Layouts 1.15
import ".."
import "../Components"

// One file of the comparison: what it is called, what it measured, and a way to choose another.
ColumnLayout {
    id: root
    spacing: 6

    required property string label
    required property string fileName
    required property string report
    required property color accentColor
    property bool busy: false

    signal openRequested
    signal clearRequested
    //! A path picked from the recent list, which needs no file dialog at all.
    signal recentRequested(string filePath)

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            width: 12
            height: 12
            radius: 2
            color: root.accentColor
        }
        Label {
            text: root.label
            color: "#dddddd"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            text: root.fileName !== "" ? root.fileName : qsTr("No file")
            color: root.fileName !== "" ? "white" : "#777777"
            elide: Text.ElideMiddle
        }
        BusyIndicator {
            running: root.busy
            visible: root.busy
            implicitWidth: 20
            implicitHeight: 20
        }
        AppButton {
            text: qsTr("Open...")
            implicitWidth: Constants.defaultButtonWidth * 0.8
            enabled: !root.busy
            onClicked: root.openRequested()
        }
        // The same files get held against one song after another, so the list is worth one click
        // rather than a walk through the file dialog every time.
        AppButton {
            text: "\u25be"
            implicitWidth: Constants.defaultButtonWidth * 0.3
            enabled: !root.busy && audioAnalysisController.recentFiles.length > 0
            onClicked: recentMenu.open()

            Menu {
                id: recentMenu
                y: parent.height
                Universal.theme: Universal.Dark
                Repeater {
                    model: audioAnalysisController.recentFiles
                    MenuItem {
                        required property int index
                        required property string modelData
                        text: audioAnalysisController.recentFileNames()[index]
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: modelData
                        onTriggered: root.recentRequested(modelData)
                    }
                }
            }
        }
        AppButton {
            text: qsTr("Clear")
            implicitWidth: Constants.defaultButtonWidth * 0.8
            enabled: root.fileName !== "" && !root.busy
            onClicked: root.clearRequested()
        }
    }

    ScrollView {
        id: reportScrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 120
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        TextArea {
            width: reportScrollView.availableWidth
            text: root.report !== "" ? root.report : ("<i>" + qsTr("Open a WAV or FLAC file to analyse it.") + "</i>")
            textFormat: Text.RichText
            wrapMode: Text.Wrap
            color: "white"
            font.pointSize: 10
            readOnly: true
            selectByMouse: true
            background: null
            activeFocusOnTab: false
        }
    }
}
