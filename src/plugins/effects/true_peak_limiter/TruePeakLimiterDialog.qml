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
    title: "<strong>" + qsTr("True Peak Limiter (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.defaultDialogScale : 640
    height: parent ? parent.height * Constants.defaultDialogScale : 480

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    property real currentReductionDb: 0.0
    property real currentTruePeakDb: -70.0
    property real currentTruePeakHoldDb: -70.0
    readonly property bool autoReleaseEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.autoRelease) > 0.5;
    }

    ScrollView {
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 14
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: dialogScrollView.availableWidth
            spacing: 12

            EffectPresetRow {
                effectIndex: root.effectIndex
                showWhenEmpty: true
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    spacing: 14
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    GridLayout {
                        columns: 2
                        columnSpacing: 24
                        rowSpacing: 14
                        Layout.fillWidth: true

                        Knob {
                            Layout.row: 0
                            Layout.column: 0
                            label: qsTr("Threshold")
                            suffix: "dB"
                            from: -24
                            to: 0
                            stepSize: 0.1
                            isInteger: false
                            value: {
                                effectRackController.revision;
                                return -24 + effectRackController.parameterValue(root.effectIndex, NahdXml.threshold) * 24;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.threshold, (v + 24) / 24)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 0
                            Layout.column: 1
                            label: qsTr("Ceiling")
                            mapping: "value"
                            suffix: "dBTP"
                            from: -6
                            to: 0
                            stepSize: 0.1
                            isInteger: false
                            value: {
                                effectRackController.revision;
                                return -6 + effectRackController.parameterValue(root.effectIndex, NahdXml.ceiling) * 6;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.ceiling, (v + 6) / 6)
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
                            enabled: !root.autoReleaseEnabled
                            opacity: root.autoReleaseEnabled ? 0.45 : 1.0
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, NahdXml.release) * 1000;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.release, v / 1000)
                            Layout.fillWidth: true
                        }

                        Knob {
                            Layout.row: 1
                            Layout.column: 1
                            label: qsTr("Lookahead")
                            suffix: "ms"
                            mapping: "value"
                            from: 0
                            to: 10
                            stepSize: 0.1
                            isInteger: false
                            value: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, NahdXml.lookahead) * 10;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.lookahead, v / 10)
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        spacing: 14
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Boost")
                            font.bold: true
                        }

                        Switch {
                            checked: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, NahdXml.boost) > 0.5;
                            }
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.boost, checked ? 1 : 0)
                        }

                        Label {
                            text: qsTr("Auto Release")
                            font.bold: true
                        }

                        Switch {
                            checked: root.autoReleaseEnabled
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.autoRelease, checked ? 1 : 0)
                        }
                    }

                    Label {
                        text: root.autoReleaseEnabled ? qsTr("4x true-peak detection enabled with BPM-synced dual-stage recovery.") : qsTr("4x true-peak detection enabled with manual single-stage recovery.")
                        color: Theme.textSecondary
                        font.italic: true
                        font.pointSize: 9
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                // Gain Reduction & True Peak Meter
                ColumnLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: 76

                    Label {
                        text: qsTr("Reduction")
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        width: 30
                        height: 118
                        color: Theme.surfaceSunken
                        border.color: Theme.borderSubtle
                        Layout.alignment: Qt.AlignHCenter

                        Rectangle {
                            anchors.top: parent.top
                            anchors.topMargin: 2
                            width: parent.width - 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: Math.min(parent.height - 4, (Math.abs(root.currentReductionDb) / 30.0) * (parent.height - 4))
                            color: themeService.accentColor
                        }

                        Item {
                            anchors.fill: parent
                            Repeater {
                                model: 7
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: Theme.separatorStrong
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

                    Label {
                        text: qsTr("True Peak")
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 4
                    }

                    Label {
                        text: root.currentTruePeakHoldDb <= -69.5 ? "--- dBTP" : (root.currentTruePeakHoldDb >= 0 ? "+" : "") + root.currentTruePeakHoldDb.toFixed(1) + " dBTP"
                        color: root.currentTruePeakHoldDb > -0.1 ? Theme.danger : Theme.textBright
                        font.family: "Monospace"
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignHCenter
                    }

                    AppButton {
                        text: qsTr("Reset Peak")
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: dialogModel.resetPeakHold()
                    }
                }
            }
        }
    }

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentReductionDb = dialogModel.reductionDb();
            root.currentTruePeakDb = dialogModel.outputTruePeakDb();
            root.currentTruePeakHoldDb = dialogModel.outputTruePeakHoldDb();
        }
    }

    TruePeakLimiterDialogModel {
        id: dialogModel
        rack: effectRackController
        effectIndex: root.effectIndex
    }
}
