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

EffectDialog {
    id: root
    title: "<strong>" + qsTr("Drive Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 550
    height: 450

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        RowLayout {
            spacing: 20
            Layout.fillWidth: true

            Label {
                text: qsTr("Algorithm")
                font.bold: true
            }

            ComboBox {
                model: [qsTr("Soft"), qsTr("Hard"), qsTr("Fold"), qsTr("Dist")]
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.driveModeKey());
                }
                onActivated: i => effectRackController.setParameterValue(root.effectIndex, effectRackController.driveModeKey(), i)
                Layout.fillWidth: true
            }
        }

        GridLayout {
            columns: 2
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Knob {
                Layout.row: 0
                Layout.column: 0
                label: qsTr("Drive")
                suffix: "dB"
                from: 0
                to: 40
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.driveAmountKey()) * 40;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.driveAmountKey(), v / 40)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 0
                Layout.column: 1
                label: qsTr("Mix")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.driveMixKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.driveMixKey(), v / 100)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 1
                Layout.column: 0
                label: qsTr("Output")
                suffix: "dB"
                from: -12
                to: 12
                value: {
                    effectRackController.revision;
                    return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.driveGainKey()) * 24;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.driveGainKey(), (v + 12) / 24)
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
