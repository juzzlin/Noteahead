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

Dialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("Panner Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 400
    height: 300

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
        spacing: 20

        GridLayout {
            columns: 2
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true

            Knob {
                label: qsTr("Pan")
                mapping: "pan"
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "pan") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "pan", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Width")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbWidth") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbWidth", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
