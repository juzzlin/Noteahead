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
import Noteahead 1.0

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Rendering audio") + "</strong>"
    modal: true
    closePolicy: Popup.NoAutoClose
    implicitWidth: 450
    implicitHeight: 200

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Rendering...")
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 1
            value: renderService.progress
        }

        Label {
            Layout.alignment: Qt.AlignRight
            text: Math.round(renderService.progress * 100) + " %"
        }
    }

    Connections {
        target: renderService
        function onRenderingFinished(success, message) {
            rootItem.close();
        }
    }

    Component.onCompleted: visible = false
}
