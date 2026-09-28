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
    title: "<strong>" + qsTr("Tremolo (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
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
                text: qsTr("Level modulated by an LFO: the Auto Panner's mechanism pointed at loudness instead of position. Unlike panning it survives a fold to mono, because it genuinely takes level away and puts it back.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            EffectPresetRow {
                effectIndex: root.effectIndex
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                ComboBoxColumn {
                    label: qsTr("Waveform")
                    model: effectRackController.lfoWaveformNames()
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.tremoloWaveformKey()));
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.tremoloWaveformKey(), index)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Depth")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.tremoloIntensityKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.tremoloIntensityKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much level the deepest point takes away. The loudest the tremolo ever is equals the signal that went in")
                }

                Knob {
                    label: qsTr("Stereo Phase")
                    suffix: "°"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 180
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.tremoloStereoPhaseKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.tremoloStereoPhaseKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("At zero both channels duck together. At a hundred and eighty they duck in opposition, and the sound swings between the speakers")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Rate")
                    suffix: " Hz"
                    mapping: "lfoFrequency"
                    mapMin: 0.05
                    mapMax: 20
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.tremoloRateKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.tremoloRateKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Used when Sync is off")
                }

                Knob {
                    label: qsTr("Rate Divider")
                    mapping: "integer"
                    suffix: ""
                    from: 1
                    to: effectRackController.tremoloMaxRateDivider()
                    stepSize: 1
                    value: {
                        effectRackController.revision;
                        return Math.max(1, effectRackController.parameterValue(root.effectIndex, effectRackController.tremoloRateDividerKey()));
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.tremoloRateDividerKey(), Math.round(v))
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Divides the rate in both modes, so the tremolo can breathe over bars rather than beats")
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
