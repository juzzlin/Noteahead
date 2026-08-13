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

    title: "<strong>" + qsTr("Auto Filter Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

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
            color: "#aaa"
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
                    currentIndex: Math.round(root.parameterValue(effectRackController.autoFilterFilterTypeKey()))
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterFilterTypeKey(), index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Slope")
                    model: [qsTr("12 dB/oct"), qsTr("24 dB/oct")]
                    currentIndex: root.parameterValue(effectRackController.autoFilterFilterSlopeKey()) > 0.5 ? 1 : 0
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterFilterSlopeKey(), index)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Cutoff")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 20
                    mapMax: 20000
                    value: root.parameterValue(effectRackController.autoFilterCutoffKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterCutoffKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Resonance")
                    suffix: "%"
                    value: root.parameterValue(effectRackController.autoFilterResonanceKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterResonanceKey(), v / Constants.uiInternalScaling)
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
                    model: effectRackController.autoFilterWaveformNames()
                    currentIndex: Math.round(root.parameterValue(effectRackController.autoFilterLfoWaveformKey()))
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterLfoWaveformKey(), index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Mode")
                    model: effectRackController.autoFilterLfoModeNames()
                    currentIndex: Math.round(root.parameterValue(effectRackController.autoFilterLfoModeKey()))
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterLfoModeKey(), index)
                    Layout.fillWidth: true
                }

                RateControl {
                    modeKey: effectRackController.autoFilterLfoModeKey()
                    rateKey: effectRackController.autoFilterLfoRateKey()
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Intensity")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(effectRackController.autoFilterLfoIntensityKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterLfoIntensityKey(), v / Constants.uiInternalScaling)
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
                    model: effectRackController.autoFilterWaveformNames()
                    currentIndex: Math.round(root.parameterValue(effectRackController.autoFilterLfo2WaveformKey()))
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterLfo2WaveformKey(), index)
                    Layout.fillWidth: true
                }

                ComboBoxColumn {
                    label: qsTr("Mode")
                    model: effectRackController.autoFilterLfoModeNames()
                    currentIndex: Math.round(root.parameterValue(effectRackController.autoFilterLfo2ModeKey()))
                    onActivated: index => root.setParameterValue(effectRackController.autoFilterLfo2ModeKey(), index)
                    Layout.fillWidth: true
                }

                RateControl {
                    modeKey: effectRackController.autoFilterLfo2ModeKey()
                    rateKey: effectRackController.autoFilterLfo2RateKey()
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Intensity")
                    mapping: "cubicCentered"
                    mapMin: -100
                    mapMax: 100
                    value: root.parameterValue(effectRackController.autoFilterLfo2IntensityKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterLfo2IntensityKey(), v / Constants.uiInternalScaling)
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
                    value: root.parameterValue(effectRackController.autoFilterEnvModKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterEnvModKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Attack")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 0.1
                    mapMax: 500
                    value: root.parameterValue(effectRackController.autoFilterEnvAttackKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterEnvAttackKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Release")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 1
                    mapMax: 2000
                    value: root.parameterValue(effectRackController.autoFilterEnvReleaseKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterEnvReleaseKey(), v / Constants.uiInternalScaling)
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
                    value: root.parameterValue(effectRackController.autoFilterStereoPhaseKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterStereoPhaseKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    mapMin: -12
                    mapMax: 12
                    value: root.parameterValue(effectRackController.autoFilterGainKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterGainKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    value: root.parameterValue(effectRackController.autoFilterMixKey()) * Constants.uiInternalScaling
                    onMoved: v => root.setParameterValue(effectRackController.autoFilterMixKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }
        }
    }
}
