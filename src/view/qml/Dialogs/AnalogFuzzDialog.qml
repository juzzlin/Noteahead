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
    title: "<strong>" + qsTr("Analog Fuzz (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.defaultDialogScale : 700
    height: parent ? parent.height * Constants.effectDialogScale : 500

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    property real currentSaturationDb: 0.0

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            Label {
                text: qsTr("A synth's drive knob: the fuzz stage is played through the filter it is overdriving, so the resonant peak gives way as Drive comes up and the harmonics come out rounded instead of sitting on top.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                GridLayout {
                    columns: 3
                    columnSpacing: 24
                    rowSpacing: 16
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    Knob {
                        label: qsTr("Drive")
                        suffix: "dB"
                        from: 0
                        to: 42
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzDriveKey()) * 42;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzDriveKey(), v / 42)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Fuzz")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzFuzzKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzFuzzKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Bias")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzBiasKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzBiasKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Cutoff")
                        suffix: "Hz"
                        mapping: "logFrequency"
                        mapMin: 60
                        mapMax: 12000
                        from: 0
                        to: 1000
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzCutoffKey()) * 1000;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzCutoffKey(), v / 1000)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Resonance")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzResonanceKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzResonanceKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Mix")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzMixKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzMixKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        label: qsTr("Output")
                        suffix: "dB"
                        from: -12
                        to: 12
                        value: {
                            effectRackController.revision;
                            return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.analogFuzzGainKey()) * 24;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.analogFuzzGainKey(), (v + 12) / 24)
                        Layout.fillWidth: true
                    }
                }

                // Saturation meter
                ColumnLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: 60

                    Label {
                        text: qsTr("Saturation")
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Rectangle {
                        color: "#111"
                        border.color: "#333"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 160
                        // The meter is the one thing here that must not be squeezed away when the
                        // dialog is short: without this the layout takes the whole deficit out of it.
                        Layout.minimumHeight: 160

                        Rectangle {
                            anchors.top: parent.top
                            anchors.topMargin: 2
                            width: parent.width - 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: Math.min(parent.height - 4, (Math.abs(root.currentSaturationDb) / 30.0) * (parent.height - 4))
                            color: themeService.accentColor
                        }

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

            Label {
                text: qsTr("Fuzz runs from a soft valve-like knee to a hard, nearly square clip. Bias sets where on that curve the signal sits: centred is odd harmonics and a hollow tone, off to either side brings in the even ones, and far out the stage starves and gates.")
                font.pixelSize: 11
                color: "#999"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                // Wrapping text must not drive the column's width, or the dialog reflows.
                Layout.preferredWidth: 0
            }
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentSaturationDb = effectRackController.analogFuzzSaturationDb(root.effectIndex);
        }
    }
}
