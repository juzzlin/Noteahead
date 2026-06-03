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

    title: "<strong>" + qsTr("AutoPanner Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 500
    height: 400

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

            ColumnLayout {
                spacing: 5
                Label {
                    text: qsTr("Waveform")
                    font.bold: true
                }
                ComboBox {
                    model: [qsTr("Saw"), qsTr("Triangle"), qsTr("Square"), qsTr("Sine")]
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, "waveform") * 3);
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, "waveform", index / 3.0)
                    Layout.fillWidth: true
                }
            }

            Knob {
                label: qsTr("Intensity")
                suffix: "%"
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "intensity") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "intensity", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignTop
                Label {
                    text: qsTr("Rate Mode")
                    font.bold: true
                }
                ComboBox {
                    model: [qsTr("Hz"), qsTr("Sync")]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "sync") > 0.5 ? 1 : 0;
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, "sync", index)
                    Layout.fillWidth: true
                }
            }

            StackLayout {
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "sync") > 0.5 ? 1 : 0;
                }
                Layout.alignment: Qt.AlignTop

                Knob {
                    label: qsTr("Rate")
                    suffix: "Hz"
                    mapping: "exponential"
                    mapMin: 0.05
                    mapMax: 20.0
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "rate") * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, "rate", v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                SyncSlider {
                    label: qsTr("Sync")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "delaySyncDivision") * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delaySyncDivision", v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
