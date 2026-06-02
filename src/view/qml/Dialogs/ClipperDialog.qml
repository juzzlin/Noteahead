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
    title: "<strong>" + qsTr("Clipper Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 500
    height: 350

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

        RowLayout {
            spacing: 20
            Layout.fillWidth: true

            Label {
                text: qsTr("Mode")
                font.bold: true
            }

            ComboBox {
                model: [qsTr("Hard"), qsTr("Soft (Tanh)")]
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.clipperModeKey());
                }
                onActivated: i => effectRackController.setParameterValue(root.effectIndex, effectRackController.clipperModeKey(), i)
                Layout.fillWidth: true
            }
        }

        Knob {
            label: qsTr("Threshold")
            suffix: "dB"
            from: -24
            to: 0
            value: {
                effectRackController.revision;
                return -24 + effectRackController.parameterValue(root.effectIndex, effectRackController.clipperThresholdKey()) * 24;
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.clipperThresholdKey(), (v + 24) / 24)
            Layout.fillWidth: true
        }

        Knob {
            label: qsTr("Gain")
            suffix: "dB"
            from: -24
            to: 24
            value: {
                effectRackController.revision;
                return -24 + effectRackController.parameterValue(root.effectIndex, effectRackController.clipperGainKey()) * 48;
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.clipperGainKey(), (v + 24) / 48)
            Layout.fillWidth: true
        }
    }
}
