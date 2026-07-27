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
    property int effectIndex: -1
    title: "<strong>" + qsTr("Simple EQ (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Item {
            Layout.fillHeight: true
        }

        Knob {
            label: qsTr("Sounds Good")
            suffix: "%"
            from: 0
            to: 100
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 140
            value: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, effectRackController.simpleEqAmountKey()) * 100;
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.simpleEqAmountKey(), v / 100)
        }

        Label {
            text: qsTr("Turn it up until it sounds good.")
            color: "#aaa"
            font.italic: true
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
