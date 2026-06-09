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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0

ColumnLayout {
    spacing: 15
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true
        Item {
            Layout.preferredWidth: resetBtn.implicitWidth
        }
        Label {
            text: qsTr("Emulation of a 16-pad sampler")
            color: "white"
            font.bold: true
            font.pointSize: 20
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
        Button {
            id: resetBtn
            text: qsTr("Reset")
            implicitWidth: Constants.defaultButtonWidth
            onClicked: samplerController.reset()
        }
    }

    Label {
        text: qsTr("Press and hold pad to play, release to stop. Right-click to clear. Assignments are saved with the song project. To use the sampler, select '%1' as the port in Track Settings.").arg(applicationService.samplerDeviceName)
        color: "#aaa"
        font.pointSize: 10
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        Layout.alignment: Qt.AlignHCenter
    }
}
