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
    title: "<strong>" + qsTr("Compressor Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 650
    height: 550

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
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 30

        GridLayout {
            columns: 3
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true

            Knob {
                label: qsTr("Threshold")
                suffix: "dB"
                from: -60
                to: 0
                value: {
                    effectRackController.revision;
                    return -60 + effectRackController.parameterValue(root.effectIndex, effectRackController.compressorThresholdKey()) * 60;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorThresholdKey(), (v + 60) / 60)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Ratio")
                suffix: ":1"
                from: 1
                to: 20
                value: {
                    effectRackController.revision;
                    return 1 + effectRackController.parameterValue(root.effectIndex, effectRackController.compressorRatioKey()) * 19;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorRatioKey(), (v - 1) / 19)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Knee")
                suffix: "dB"
                from: 0
                to: 24
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.compressorKneeKey()) * 24;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorKneeKey(), v / 24)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Attack")
                suffix: "ms"
                mapping: "exponential"
                mapMin: 0.1
                mapMax: 500
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.compressorAttackKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorAttackKey(), v / 1000)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Release")
                suffix: "ms"
                mapping: "exponential"
                mapMin: 1.0
                mapMax: 2000
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.compressorReleaseKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorReleaseKey(), v / 1000)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Makeup")
                suffix: "dB"
                from: -12
                to: 12
                value: {
                    effectRackController.revision;
                    return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.compressorMakeupKey()) * 24;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorMakeupKey(), (v + 12) / 24)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Lookahead")
                suffix: "ms"
                from: 0
                to: 10
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.compressorLookaheadKey()) * 10;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorLookaheadKey(), v / 10)
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
                height: 300
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
        } // RowLayout

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label {
                text: qsTr("Side Chain Source:")
                font.bold: true
            }
            ComboBox {
                id: sideChainCombo
                Layout.fillWidth: true
                model: {
                    var items = [qsTr("None")];
                    for (var i = 0; i < deviceRackController.deviceCount; i++) {
                        items.push(qsTr("Device %1").arg(i + 1));
                    }
                    return items;
                }
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.compressorSideChainSourceDeviceKey()) + 1;
                }
                onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.compressorSideChainSourceDeviceKey(), index - 1)
            }
        }
    } // ColumnLayout

    property real currentReductionDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentReductionDb = effectRackController.compressorReductionDb(root.effectIndex);
        }
    }
}
