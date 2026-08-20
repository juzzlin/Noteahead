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
    title: "<strong>" + qsTr("Saturator Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
            Layout.fillHeight: true

            ColumnLayout {
                spacing: 20
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    spacing: 20
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Type")
                        font.bold: true
                    }

                    ComboBox {
                        model: [qsTr("Tape"), qsTr("Tube"), qsTr("Diode")]
                        currentIndex: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.saturatorModeKey());
                        }
                        onActivated: i => effectRackController.setParameterValue(root.effectIndex, effectRackController.saturatorModeKey(), i)
                        Layout.fillWidth: true
                    }
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 30
                    rowSpacing: 20
                    Layout.fillWidth: true

                    Knob {
                        Layout.row: 0
                        Layout.column: 0
                        label: qsTr("Drive")
                        suffix: "dB"
                        from: 0
                        to: 40
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.saturatorDriveKey()) * 40;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.saturatorDriveKey(), v / 40)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 0
                        Layout.column: 1
                        label: qsTr("Tone")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.saturatorToneKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.saturatorToneKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 1
                        Layout.column: 0
                        label: qsTr("Mix")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.saturatorMixKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.saturatorMixKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 1
                        Layout.column: 1
                        label: qsTr("Output")
                        suffix: "dB"
                        from: -12
                        to: 12
                        value: {
                            effectRackController.revision;
                            return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.saturatorGainKey()) * 24;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.saturatorGainKey(), (v + 12) / 24)
                        Layout.fillWidth: true
                    }
                }
            }

            // Saturation Meter
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignTop
                Label {
                    text: qsTr("Saturation")
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Rectangle {
                    id: meterContainer
                    width: 30
                    height: 200
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    Rectangle {
                        id: meterFill
                        anchors.top: parent.top
                        anchors.topMargin: 2
                        width: parent.width - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: Math.min(parent.height - 4, (Math.abs(root.currentSaturationDb) / 30.0) * (parent.height - 4))
                        color: themeService.accentColor
                    }

                    // Tick marks
                    Item {
                        anchors.fill: parent
                        anchors.margins: 2
                        Repeater {
                            model: 7 // 0, -5, -10, -15, -20, -25, -30
                            Rectangle {
                                width: parent.width
                                height: 1
                                color: "#555"
                                opacity: 0.5
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: (index / 6) * parent.height
                            }
                        }
                    }
                }
                Label {
                    text: root.currentSaturationDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    property real currentSaturationDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentSaturationDb = effectRackController.saturatorSaturationDb(root.effectIndex);
        }
    }
}
