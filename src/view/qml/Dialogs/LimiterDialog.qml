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
    title: "<strong>" + qsTr("Limiter Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 560
    height: 460

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
                spacing: 30
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    spacing: 20
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    GridLayout {
                        columns: 2
                        columnSpacing: 30
                        rowSpacing: 20
                        Layout.fillWidth: true

                        Knob {
                            Layout.row: 0
                            Layout.column: 0
                            label: qsTr("Threshold")
                            suffix: "dB"
                            from: -24
                            to: 0
                            value: {
                                effectRackController.revision;
                                return -24 + effectRackController.parameterValue(root.effectIndex, effectRackController.limiterThresholdKey()) * 24;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.limiterThresholdKey(), (v + 24) / 24)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 0
                            Layout.column: 1
                            label: qsTr("Ceiling")
                            suffix: "dB"
                            from: -3
                            to: 0
                            stepSize: 0.1
                            value: {
                                effectRackController.revision;
                                return -3 + effectRackController.parameterValue(root.effectIndex, effectRackController.limiterCeilingKey()) * 3;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.limiterCeilingKey(), (v + 3) / 3)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 0
                            label: qsTr("Release")
                            suffix: "ms"
                            mapping: "exponential"
                            mapMin: 1.0
                            mapMax: 1000
                            from: 0
                            to: 1000
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.limiterReleaseKey()) * 1000;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.limiterReleaseKey(), v / 1000)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 1
                            label: qsTr("Lookahead")
                            suffix: "ms"
                            from: 0
                            to: 10
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.limiterLookaheadKey()) * 10;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.limiterLookaheadKey(), v / 10)
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        spacing: 20
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Boost")
                            font.bold: true
                        }

                        Switch {
                            checked: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, effectRackController.limiterBoostKey()) > 0.5;
                            }
                            onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.limiterBoostKey(), checked ? 1 : 0)
                        }

                        Label {
                            text: qsTr("Lift threshold to ceiling for maximum loudness")
                            color: "#aaa"
                            font.italic: true
                            font.pointSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
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
                        height: 260
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
    }

    property real currentReductionDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentReductionDb = effectRackController.limiterReductionDb(root.effectIndex);
        }
    }
}
