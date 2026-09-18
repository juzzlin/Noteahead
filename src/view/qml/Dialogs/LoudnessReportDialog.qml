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
import "../Components"

AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Loudness Analysis Report") + "</strong>"
    modal: true
    // Sized here rather than where it is instantiated, and larger than the old half-window: the
    // report is prose now as well as tables, and half of a 1024-wide window breaks these sentences
    // over four lines each.
    width: parent ? parent.width * Constants.defaultDialogScale : 720
    height: parent ? parent.height * Constants.defaultDialogScale : 540

    property alias reportText: reportLabel.text

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    background: Rectangle {
        color: "#222"
        border.color: "#444"
        radius: 10
    }

    ScrollView {
        id: reportScrollView
        anchors.fill: parent
        anchors.margins: 15
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        TextArea {
            id: reportLabel
            width: reportScrollView.availableWidth
            textFormat: Text.RichText
            wrapMode: Text.Wrap
            color: "white"
            font.pointSize: 12
            readOnly: true
            selectByMouse: true
            background: null
            activeFocusOnTab: false
        }
    }

    Component.onCompleted: visible = false
}
