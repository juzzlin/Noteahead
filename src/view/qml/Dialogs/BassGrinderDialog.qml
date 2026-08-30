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
    property real currentSaturationDb: 0.0
    title: "<strong>" + qsTr("Bass Grinder Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 700
    height: 560

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 2
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: dialogScrollView.availableWidth
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
                            text: qsTr("Color")
                            font.bold: true
                        }

                        ComboBox {
                            model: [qsTr("Off"), qsTr("On")]
                            currentIndex: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderColorKey());
                            }
                            onActivated: i => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderColorKey(), i)
                            Layout.fillWidth: true
                        }
                    }

                    GridLayout {
                        columns: 3
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
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderDriveKey()) * 40;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderDriveKey(), v / 40)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 0
                            Layout.column: 1
                            label: qsTr("Blend")
                            suffix: "%"
                            from: 0
                            to: 100
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderBlendKey()) * 100;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderBlendKey(), v / 100)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 0
                            Layout.column: 2
                            label: qsTr("Split")
                            suffix: "Hz"
                            mapping: "logFrequency"
                            mapMin: 20
                            mapMax: 800
                            from: 0
                            to: 1000
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderSplitFreqKey()) * 1000;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderSplitFreqKey(), v / 1000)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 0
                            label: qsTr("Bass")
                            suffix: "dB"
                            from: -15
                            to: 15
                            value: {
                                effectRackController.revision;
                                return -15 + effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderBassGainKey()) * 30;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderBassGainKey(), (v + 15) / 30)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 1
                            label: qsTr("Mid")
                            suffix: "dB"
                            from: -15
                            to: 15
                            value: {
                                effectRackController.revision;
                                return -15 + effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderMidGainKey()) * 30;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderMidGainKey(), (v + 15) / 30)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 2
                            label: qsTr("Mid Freq")
                            suffix: "Hz"
                            mapping: "logFrequency"
                            mapMin: 200
                            mapMax: 3000
                            from: 0
                            to: 1000
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderMidFreqKey()) * 1000;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderMidFreqKey(), v / 1000)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 2
                            Layout.column: 0
                            label: qsTr("Treble")
                            suffix: "dB"
                            from: -15
                            to: 15
                            value: {
                                effectRackController.revision;
                                return -15 + effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderHighGainKey()) * 30;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderHighGainKey(), (v + 15) / 30)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 2
                            Layout.column: 1
                            label: qsTr("Mix")
                            suffix: "%"
                            from: 0
                            to: 100
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderMixKey()) * 100;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderMixKey(), v / 100)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 2
                            Layout.column: 2
                            label: qsTr("Output")
                            suffix: "dB"
                            from: -12
                            to: 12
                            value: {
                                effectRackController.revision;
                                return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.bassGrinderGainKey()) * 24;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bassGrinderGainKey(), (v + 12) / 24)
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: qsTr("Split takes everything below it out of the clipper, so the fundamental of a kick or a bass comes through whole while the band above it is ground up. Blend sets how much of that band the clipper replaces. At the bottom of its travel Split is below the audible range and the whole signal is distorted.")
                        font.pixelSize: 11
                        color: "#999"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        // Wrapping text must not drive the column's width, or the dialog reflows.
                        Layout.preferredWidth: 0
                    }
                }

                // Saturation meter
                ColumnLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignTop
                    Label {
                        text: qsTr("Saturation")
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Rectangle {
                        width: 30
                        height: 200
                        color: "#111"
                        border.color: "#333"
                        Layout.alignment: Qt.AlignHCenter

                        Rectangle {
                            anchors.top: parent.top
                            anchors.topMargin: 2
                            width: parent.width - 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: Math.min(parent.height - 4, (Math.abs(root.currentSaturationDb) / 30.0) * (parent.height - 4))
                            color: themeService.accentColor
                        }

                        // Tick marks
                        Item {
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
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentSaturationDb = effectRackController.bassGrinderSaturationDb(root.effectIndex);
        }
    }
}
