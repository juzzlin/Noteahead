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
    title: "<strong>" + qsTr("Room Mics (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    RoomMicsDialogModel {
        id: dialogModel
        rack: effectRackController
        effectIndex: root.effectIndex
    }

    readonly property var roomLayout: {
        // The slot is set right before the dialog opens, after this was first worked out
        effectRackController.revision;
        dialogModel.effectIndex;
        return dialogModel.layout();
    }

    function parameterValue(key: string): real {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, key);
    }
    function setParameterValue(key: string, value: real): void {
        effectRackController.setParameterValue(root.effectIndex, key, value);
    }

    //! A knob on one of the effect's parameters, which run 0..1 and are shown over 0..scale.
    component ParameterKnob: Knob {
        property string key
        property real scale: 100
        from: 0
        to: scale
        value: root.parameterValue(key) * scale
        onMoved: v => root.setParameterValue(key, v / scale)
        Layout.fillWidth: true
    }

    //! A list of choices on a discrete parameter, with its name over it.
    component ParameterChoice: ColumnLayout {
        property string key
        property alias label: choiceLabel.text
        property var choices: []
        spacing: 2
        Layout.fillWidth: true
        Label {
            id: choiceLabel
            color: Theme.textSecondary
            font.pixelSize: 12
        }
        ComboBox {
            model: parent.choices
            currentIndex: Math.round(root.parameterValue(parent.key))
            onActivated: index => root.setParameterValue(parent.key, index)
            Layout.fillWidth: true
        }
    }

    component SectionLabel: Label {
        font.bold: true
        color: "#ccc"
        Layout.topMargin: 4
    }

    readonly property var materials: {
        // A JS block is not a translation binding: reading the language declares the dependency
        languageService.activeLanguage;
        return [qsTr("Stone"), qsTr("Concrete"), qsTr("Brick"), qsTr("Plaster"), qsTr("Wood"), qsTr("Glass"), qsTr("Carpet"), qsTr("Curtain"), qsTr("Acoustic Panel"), qsTr("Audience")];
    }
    readonly property var patterns: {
        languageService.activeLanguage;
        return [qsTr("Omni"), qsTr("Cardioid"), qsTr("Figure-8")];
    }
    readonly property var groupNames: {
        languageService.activeLanguage;
        return [qsTr("Close"), qsTr("Room"), qsTr("Far")];
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 12

            EffectPresetRow {
                effectIndex: root.effectIndex
                showWhenEmpty: true
            }

            Label {
                text: qsTr("Records the sound again in a room: drag the source and the mics. Shift picks up the Close mics while they stand on the source.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RoomViewRenderer {
                layout: root.roomLayout
                accentColor: themeService.accentColor
                sourceName: effectRackController.targetDeviceName
                groupNames: root.groupNames
                onSourceDragged: (x, y) => dialogModel.moveSource(x, y)
                onMicsDragged: (group, x, y) => dialogModel.moveMics(group, x, y)
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                Layout.minimumHeight: 200
            }

            Label {
                text: root.roomLayout.width === undefined ? "" : root.roomLayout.width.toFixed(1) + " x " + root.roomLayout.depth.toFixed(1) + " x " + root.roomLayout.height.toFixed(1) + " m, RT60 " + root.roomLayout.reverbTime.toFixed(2) + " s"
                color: Theme.textSecondary
                font.pixelSize: 12
            }

            SectionLabel {
                text: qsTr("Room")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Width")
                    suffix: "m"
                    mapping: "exponential"
                    mapMin: 2
                    mapMax: 50
                    isInteger: false
                    scale: 1000
                    key: NahdXml.roomWidth
                }
                ParameterKnob {
                    label: qsTr("Depth")
                    suffix: "m"
                    mapping: "exponential"
                    mapMin: 2
                    mapMax: 50
                    isInteger: false
                    scale: 1000
                    key: NahdXml.roomDepth
                }
                ParameterKnob {
                    label: qsTr("Height")
                    suffix: "m"
                    mapping: "exponential"
                    mapMin: 2
                    mapMax: 20
                    isInteger: false
                    scale: 1000
                    key: NahdXml.roomHeight
                }
                ParameterKnob {
                    label: qsTr("Spread")
                    suffix: "m"
                    mapMin: 0
                    mapMax: 10
                    isInteger: false
                    scale: 1000
                    key: NahdXml.spread
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far apart the left and the right of the input play, as two speakers")
                }
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterChoice {
                    label: qsTr("Walls")
                    key: NahdXml.wallMaterial
                    choices: root.materials
                }
                ParameterChoice {
                    label: qsTr("Floor")
                    key: NahdXml.floorMaterial
                    choices: root.materials
                }
                ParameterChoice {
                    label: qsTr("Ceiling")
                    key: NahdXml.ceilingMaterial
                    choices: root.materials
                }
            }

            SectionLabel {
                text: qsTr("Mics")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                Repeater {
                    model: 3
                    ColumnLayout {
                        id: micColumn
                        required property int index
                        readonly property bool hasPairControls: index > 0
                        spacing: 8
                        // Equal columns, whatever each holds
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        ParameterChoice {
                            label: root.groupNames[micColumn.index]
                            key: NahdXml.micPattern(micColumn.index)
                            choices: root.patterns
                        }
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true
                            ParameterKnob {
                                label: qsTr("Level")
                                mapping: "fader"
                                key: NahdXml.micLevel(micColumn.index)
                            }
                            ParameterKnob {
                                visible: micColumn.hasPairControls
                                label: qsTr("Spacing")
                                suffix: "m"
                                mapMin: 0
                                mapMax: 10
                                isInteger: false
                                scale: 1000
                                key: micColumn.hasPairControls ? NahdXml.micSpacing(micColumn.index) : ""
                            }
                            ParameterKnob {
                                visible: micColumn.hasPairControls
                                label: qsTr("Angle")
                                suffix: "°"
                                mapMin: 0
                                mapMax: 90
                                scale: 1000
                                key: micColumn.hasPairControls ? NahdXml.micAngle(micColumn.index) : ""
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("How far each mic of the pair turns outwards from the source")
                            }
                        }
                    }
                }
            }

            SectionLabel {
                text: qsTr("Output")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Mix")
                    suffix: "%"
                    key: NahdXml.mix
                    Layout.fillWidth: false
                    Layout.preferredWidth: 120
                }
                Switch {
                    text: qsTr("Solo")
                    checked: root.parameterValue(NahdXml.solo) > 0.5
                    onToggled: root.setParameterValue(NahdXml.solo, checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only what the effect changes, without the input")
                    Layout.alignment: Qt.AlignVCenter
                }
                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
