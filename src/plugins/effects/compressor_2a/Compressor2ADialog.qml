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
    title: "<strong>" + qsTr("Compressor 2A (Slot %1)").arg(effectIndex + 1) + "</strong>"
    width: parent ? parent.width * Constants.defaultDialogScale : 700
    height: parent ? parent.height * Constants.defaultDialogScale : 560
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    //! The reduction the meter's full width stands for, in dB.
    readonly property real meterRangeDb: 20
    property real currentReductionDb: 0.0

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

            EffectPresetRow {
                effectIndex: root.effectIndex
                showWhenEmpty: true
            }

            Label {
                text: qsTr("An optical leveler: a lamp lit by the signal and a light-dependent cell that follows it. It pulls down in about 10 ms and lets go in two stages, half within some 60 ms and the rest over seconds, slower the longer it has been held down.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true
                Label {
                    text: qsTr("Mode")
                    font.bold: true
                }
                ComboBox {
                    // The unit's own switch labels, left in English
                    model: ["Compress", "Limit"]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.mode);
                    }
                    onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.mode, i)
                    Layout.preferredWidth: 160
                }
                Label {
                    text: qsTr("Gain Reduction")
                    font.bold: true
                    Layout.leftMargin: 12
                }
                // Filling from the left, as on the unit's meter, with a tick every 5 dB
                Rectangle {
                    color: Theme.surfaceSunken
                    border.color: Theme.borderSubtle
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        height: parent.height - 4
                        width: Math.min(parent.width - 4, Math.abs(root.currentReductionDb) / root.meterRangeDb * (parent.width - 4))
                        color: themeService.accentColor
                    }
                    Repeater {
                        model: 5
                        Rectangle {
                            width: 1
                            height: parent.height - 4
                            y: 2
                            x: 2 + (index / 4) * (parent.width - 5)
                            color: Theme.separatorStrong
                            opacity: 0.5
                        }
                    }
                }
                Label {
                    text: root.currentReductionDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: 70
                }
            }

            GridLayout {
                columns: 2
                columnSpacing: 24
                rowSpacing: 16
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Peak Reduction")
                    mapping: "integer"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.peakReduction) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.peakReduction, v / 100)
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How hard the signal lights the cell: both where it starts to pull down and how far")
                }

                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    from: -20
                    to: 40
                    isInteger: false
                    value: {
                        effectRackController.revision;
                        return -20 + effectRackController.parameterValue(root.effectIndex, NahdXml.gain) * 60;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.gain, (v + 20) / 60)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.mix) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.mix, v / 100)
                    Layout.fillWidth: true
                }

                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.solo) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.solo, checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only what the compressor changes, so it can be heard on its own")
                }
            }
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: root.currentReductionDb = effectRackController.compressor2AReductionDb(root.effectIndex)
    }
}
