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

    title: "<strong>" + qsTr("Phaser Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    height: parent ? parent.height * Constants.defaultDialogScale : 600
    width: parent ? parent.width * Constants.defaultDialogScale : 800
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    function parameterValue(key) {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, key);
    }

    function setParameterValue(key, value) {
        effectRackController.setParameterValue(root.effectIndex, key, value);
    }

    component SectionLabel: Label {
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    component ComboBoxColumn: ColumnLayout {
        id: comboBoxColumn
        property string label: ""
        property alias model: comboBox.model
        property int currentIndex: 0
        signal activated(int index)
        spacing: 5
        Layout.alignment: Qt.AlignTop
        Label {
            text: comboBoxColumn.label
            font.bold: true
            font.pixelSize: 11
            color: Theme.textSecondary
        }
        ComboBox {
            id: comboBox
            currentIndex: comboBoxColumn.currentIndex
            onActivated: index => comboBoxColumn.activated(index)
            Layout.fillWidth: true
        }
    }

    ScrollView {
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: dialogScrollView.availableWidth
            spacing: 10

            SectionLabel {
                text: qsTr("Cascade")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                ComboBoxColumn {
                    label: qsTr("Stages")
                    // Even counts only: one notch takes a pair of all-pass sections
                    model: {
                        // A JS block is not a translation binding, so a language change does not re-evaluate it
                        // on its own. Reading the active language makes the dependency explicit.
                        languageService.activeLanguage;
                        const stages = [];
                        for (let i = 2; i <= effectRackController.parameterMaximum(root.effectIndex, NahdXml.stages); i += 2) {
                            stages.push(qsTr("%1 stages").arg(i));
                        }
                        return stages;
                    }
                    currentIndex: Math.round(root.parameterValue(NahdXml.stages) / 2) - 1
                    onActivated: index => root.setParameterValue(NahdXml.stages, (index + 1) * 2)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Center")
                    suffix: "Hz"
                    mapping: "exponential"
                    mapMin: 50
                    mapMax: 8000
                    value: root.parameterValue(NahdXml.frequency) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.frequency, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Depth")
                    suffix: "%"
                    value: root.parameterValue(NahdXml.depth) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.depth, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Feedback")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(NahdXml.feedback) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.feedback, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Sweep")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                ComboBoxColumn {
                    label: qsTr("Waveform")
                    model: effectRackController.lfoWaveformNames()
                    currentIndex: Math.round(root.parameterValue(NahdXml.lfoWaveform))
                    onActivated: index => root.setParameterValue(NahdXml.lfoWaveform, index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Mode")
                    model: effectRackController.lfoModeNames()
                    currentIndex: Math.round(root.parameterValue(NahdXml.lfoMode))
                    onActivated: index => root.setParameterValue(NahdXml.lfoMode, index)
                    Layout.fillWidth: true
                }

                StackLayout {
                    // Only the BPM mode counts in divisions; Normal and One-Shot both run in Hz.
                    currentIndex: Math.round(root.parameterValue(NahdXml.lfoMode)) === 1 ? 1 : 0
                    Layout.fillWidth: true

                    Knob {
                        label: qsTr("Rate")
                        suffix: "Hz"
                        mapping: "lfoFrequency"
                        value: root.parameterValue(NahdXml.lfoRate) * Constants.uiInternalScaling
                        onMoved: v => root.setParameterValue(NahdXml.lfoRate, v / Constants.uiInternalScaling)
                        Layout.fillWidth: true
                    }

                    SyncSlider {
                        label: qsTr("Rate")
                        value: root.parameterValue(NahdXml.lfoRate) * Constants.uiInternalScaling
                        onMoved: v => root.setParameterValue(NahdXml.lfoRate, v / Constants.uiInternalScaling)
                        Layout.fillWidth: true
                    }
                }

                Knob {
                    label: qsTr("Rate Divider")
                    mapping: "integer"
                    suffix: ""
                    from: 1
                    to: effectRackController.parameterMaximum(root.effectIndex, NahdXml.rateDivider)
                    stepSize: 1
                    value: Math.max(1, root.parameterValue(NahdXml.rateDivider))
                    onMoved: v => root.setParameterValue(NahdXml.rateDivider, Math.round(v))
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Stereo Phase")
                    suffix: "°"
                    mapMin: 0
                    mapMax: 180
                    value: root.parameterValue(NahdXml.stereoPhase) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.stereoPhase, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Output")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    mapMin: -12
                    mapMax: 12
                    value: root.parameterValue(NahdXml.gain) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.gain, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    value: root.parameterValue(NahdXml.mix) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.mix, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Rate Divider divides the Rate by anything from 1 to %1, in both modes: a tempo-locked sweep can be stretched over several bars, and a free-running one over minutes.").arg(effectRackController.parameterMaximum(root.effectIndex, NahdXml.rateDivider))
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: Theme.textSecondary
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("The notches are deepest at 50 % Mix, where the dry and the swept signal meet at equal amounts. Feedback sharpens the peaks between them, and its two polarities cancel at different frequencies.")
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: Theme.textSecondary
                Layout.fillWidth: true
                Layout.topMargin: 10
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
