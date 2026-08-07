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

AnimatedDialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("Stereo Exciter Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 640
    height: 520

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

        Label {
            text: qsTr("Generates harmonics of the band above Tune and adds them back, for presence an equalizer cannot give: it makes top end where there is none to lift. Timbre runs odd harmonics at one end for edge, even at the other for warmth.")
            color: "#aaa"
            font.italic: true
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 20
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                columns: 2
                columnSpacing: 30
                rowSpacing: 16
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                    Knob {
                        Layout.row: 0
                        Layout.column: 0
                        label: qsTr("Tune")
                        suffix: "Hz"
                        mapping: "logFrequency"
                        mapMin: 700
                        mapMax: 8000
                        from: 0
                        to: 1000
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterTuneKey()) * 1000;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterTuneKey(), v / 1000)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 0
                        Layout.column: 1
                        label: qsTr("Peak")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterPeakKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterPeakKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 1
                        Layout.column: 0
                        label: qsTr("Zero Fill")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterZeroFillKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterZeroFillKey(), v / 100)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 1
                        Layout.column: 1
                        label: qsTr("Timbre")
                        suffix: ""
                        from: -50
                        to: 50
                        value: {
                            effectRackController.revision;
                            return (effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterTimbreKey()) - 0.5) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterTimbreKey(), v / 100 + 0.5)
                        Layout.fillWidth: true
                    }

                    Knob {
                        Layout.row: 2
                        Layout.column: 0
                        label: qsTr("Harmonics")
                        suffix: "%"
                        from: 0
                        to: 100
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterHarmonicsKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterHarmonicsKey(), v / 100)
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
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterMixKey()) * 100;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterMixKey(), v / 100)
                        Layout.fillWidth: true
                    }

            }

            ColumnLayout {
                spacing: 10
                Layout.alignment: Qt.AlignTop

                Label {
                    text: qsTr("Harmonics")
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    width: 30
                    height: 180
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 2
                        width: parent.width - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        // -60 dB at the bottom of the scale, 0 dB at the top.
                        height: Math.max(0, Math.min(parent.height - 4, (1 + root.currentHarmonicsDb / 60.0) * (parent.height - 4)))
                        color: themeService.accentColor
                    }
                }

                Label {
                    text: root.currentHarmonicsDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        CheckBox {
            text: qsTr("Solo Mode")
            checked: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoExciterSoloKey()) > 0.5;
            }
            onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoExciterSoloKey(), checked ? 1 : 0)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Pass only the harmonics being generated, so they can be heard on their own")
            Layout.fillWidth: true
        }
    }

    property real currentHarmonicsDb: -120.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentHarmonicsDb = effectRackController.stereoExciterHarmonicsDb(root.effectIndex);
        }
    }
}
