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

    title: "<strong>" + qsTr("Auto Filter Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    height: parent ? parent.height * Constants.defaultDialogScale : 600
    // Four sections of controls rather than the row or two the other effect dialogs hold, so it
    // is sized like the rack dialogs instead.
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

    //! A rate control that reads out in Hz or in beat divisions, depending on the LFO's mode.
    component RateControl: StackLayout {
        id: rateControl
        property string modeKey: ""
        property string rateKey: ""
        // Only the BPM mode counts in divisions; Normal and One-Shot both run in Hz.
        currentIndex: Math.round(root.parameterValue(rateControl.modeKey)) === 1 ? 1 : 0
        Knob {
            label: qsTr("Rate")
            suffix: "Hz"
            mapping: "lfoFrequency"
            value: root.parameterValue(rateControl.rateKey) * Constants.uiInternalScaling
            onMoved: v => root.setParameterValue(rateControl.rateKey, v / Constants.uiInternalScaling)
            Layout.fillWidth: true
        }
        SyncSlider {
            label: qsTr("Rate")
            value: root.parameterValue(rateControl.rateKey) * Constants.uiInternalScaling
            onMoved: v => root.setParameterValue(rateControl.rateKey, v / Constants.uiInternalScaling)
            Layout.fillWidth: true
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 10

            SectionLabel {
                text: qsTr("Filter")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                ComboBoxColumn {
                    label: qsTr("Type")
                    model: [qsTr("Low Pass"), qsTr("High Pass"), qsTr("Band Pass"), qsTr("Notch")]
                    currentIndex: Math.round(root.parameterValue(NahdXml.filterType))
                    onActivated: index => root.setParameterValue(NahdXml.filterType, index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Slope")
                    model: [qsTr("12 dB/oct"), qsTr("24 dB/oct")]
                    currentIndex: root.parameterValue(NahdXml.filterSlope) > 0.5 ? 1 : 0
                    onActivated: index => root.setParameterValue(NahdXml.filterSlope, index)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Cutoff")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 20
                    mapMax: 20000
                    value: root.parameterValue(NahdXml.cutoff) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.cutoff, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Resonance")
                    suffix: "%"
                    value: root.parameterValue(NahdXml.resonance) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.resonance, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Cutoff LFO")
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

                RateControl {
                    modeKey: NahdXml.lfoMode
                    rateKey: NahdXml.lfoRate
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Intensity")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(NahdXml.lfoIntensity) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.lfoIntensity, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Resonance LFO")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                ComboBoxColumn {
                    label: qsTr("Waveform")
                    model: effectRackController.lfoWaveformNames()
                    currentIndex: Math.round(root.parameterValue(NahdXml.lfo2Waveform))
                    onActivated: index => root.setParameterValue(NahdXml.lfo2Waveform, index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Mode")
                    model: effectRackController.lfoModeNames()
                    currentIndex: Math.round(root.parameterValue(NahdXml.lfo2Mode))
                    onActivated: index => root.setParameterValue(NahdXml.lfo2Mode, index)
                    Layout.fillWidth: true
                }

                RateControl {
                    modeKey: NahdXml.lfo2Mode
                    rateKey: NahdXml.lfo2Rate
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Intensity")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(NahdXml.lfo2Intensity) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.lfo2Intensity, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Envelope Follower")
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Amount")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(NahdXml.envMod) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.envMod, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Attack")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 0.1
                    mapMax: 500
                    value: root.parameterValue(NahdXml.attack) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.attack, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Release")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 1
                    mapMax: 2000
                    value: root.parameterValue(NahdXml.release) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.release, v / Constants.uiInternalScaling)
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
                    label: qsTr("Stereo Phase")
                    suffix: "°"
                    mapMin: 0
                    mapMax: 180
                    value: root.parameterValue(NahdXml.stereoPhase) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(NahdXml.stereoPhase, v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

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
            }
        }
    }
}
