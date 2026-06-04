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

            // Gain Reduction Meter
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignTop
                Label {
                    text: qsTr("Reduction")
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
                        height: Math.min(parent.height - 4, (Math.abs(root.currentReductionDb) / 30.0) * (parent.height - 4))
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
                    text: root.currentReductionDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    property real currentReductionDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentReductionDb = effectRackController.clipperReductionDb(root.effectIndex);
        }
    }
}
