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

    title: "<strong>" + qsTr("RTA — Slot %1").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 700
    height: 480

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    property var fpsIntervals: [67, 33, 16]

    Timer {
        id: updateTimer
        interval: root.fpsIntervals[fpsCombo.currentIndex]
        running: root.visible && root.effectIndex >= 0
        repeat: true
        onTriggered: {
            renderer.bands = effectRackController.rtaBandMagnitudes(root.effectIndex);
            renderer.bandPositions = effectRackController.rtaBandLogPositions(root.effectIndex);
        }
    }

    onVisibleChanged: {
        if (effectIndex >= 0) {
            effectRackController.rtaSetActive(effectIndex, visible);
        }
        if (visible && effectIndex >= 0) {
            syncRendererParams();
        }
    }

    function syncRendererParams() {
        renderer.dbRange = dbRangeValue();
        renderer.showPinkNoise = effectRackController.parameterValue(effectIndex, effectRackController.rtaShowPinkNoiseKey()) >= 0.5;
        renderer.pinkNoiseLevel = effectRackController.parameterValue(effectIndex, effectRackController.rtaPinkNoiseLevelKey());
        renderer.bandPositions = effectRackController.rtaBandLogPositions(effectIndex);
    }

    function dbRangeValue() {
        const idx = Math.round(effectRackController.parameterValue(effectIndex, effectRackController.rtaDbRangeKey()));
        const ranges = [30, 45, 60, 80];
        return ranges[Math.max(0, Math.min(3, idx))];
    }

    contentItem: ColumnLayout {
        spacing: 8
        anchors.fill: parent
        anchors.margins: 10

        RtaRenderer {
            id: renderer
            Layout.fillWidth: true
            Layout.fillHeight: true
            dbRange: 60
            showPinkNoise: true
            pinkNoiseLevel: -18.0
            accentColor: Universal.accent
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            // Band count
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("Bands"); color: "#aaaaaa"; font.pixelSize: 11 }
                ComboBox {
                    id: bandCountCombo
                    model: ["32", "64", "128"]
                    currentIndex: Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.rtaBandCountKey()))
                    onActivated: {
                        effectRackController.setParameterValue(root.effectIndex, effectRackController.rtaBandCountKey(), currentIndex);
                    }
                    Universal.theme: Universal.Dark
                    implicitWidth: 80
                }
            }

            // dB range
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("dB Range"); color: "#aaaaaa"; font.pixelSize: 11 }
                ComboBox {
                    id: dbRangeCombo
                    model: ["-30 dB", "-45 dB", "-60 dB", "-80 dB"]
                    currentIndex: Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.rtaDbRangeKey()))
                    onActivated: {
                        effectRackController.setParameterValue(root.effectIndex, effectRackController.rtaDbRangeKey(), currentIndex);
                        const ranges = [30, 45, 60, 80];
                        renderer.dbRange = ranges[currentIndex];
                    }
                    Universal.theme: Universal.Dark
                    implicitWidth: 100
                }
            }

            // Speed
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("Speed"); color: "#aaaaaa"; font.pixelSize: 11 }
                ComboBox {
                    id: speedCombo
                    model: [qsTr("Fast"), qsTr("Normal"), qsTr("Slow")]
                    currentIndex: Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.rtaSpeedKey()))
                    onActivated: {
                        effectRackController.setParameterValue(root.effectIndex, effectRackController.rtaSpeedKey(), currentIndex);
                    }
                    Universal.theme: Universal.Dark
                    implicitWidth: 90
                }
            }

            // FFT Rate
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("FFT Rate"); color: "#aaaaaa"; font.pixelSize: 11 }
                ComboBox {
                    id: fftRateCombo
                    model: [qsTr("Fast"), qsTr("Normal"), qsTr("Slow")]
                    currentIndex: Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.rtaFftRateKey()))
                    onActivated: {
                        effectRackController.setParameterValue(root.effectIndex, effectRackController.rtaFftRateKey(), currentIndex);
                    }
                    Universal.theme: Universal.Dark
                    implicitWidth: 90
                }
            }

            // FPS
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("FPS"); color: "#aaaaaa"; font.pixelSize: 11 }
                ComboBox {
                    id: fpsCombo
                    model: ["15", "30", "60"]
                    currentIndex: 1
                    Universal.theme: Universal.Dark
                    implicitWidth: 70
                }
            }

            // Pink noise toggle
            ColumnLayout {
                spacing: 2
                Label { text: qsTr("Pink Noise"); color: "#aaaaaa"; font.pixelSize: 11 }
                CheckBox {
                    id: pinkNoiseCheck
                    checked: renderer.showPinkNoise
                    onToggled: {
                        renderer.showPinkNoise = checked;
                        effectRackController.setParameterValue(root.effectIndex,
                            effectRackController.rtaShowPinkNoiseKey(), checked ? 1.0 : 0.0);
                    }
                    Universal.theme: Universal.Dark
                }
            }

            // Pink noise level
            ColumnLayout {
                spacing: 2
                enabled: renderer.showPinkNoise
                Label { text: qsTr("Pink Level"); color: "#aaaaaa"; font.pixelSize: 11 }
                Knob {
                    id: pinkLevelKnob
                    label: ""
                    suffix: "dB"
                    from: -80
                    to: 0
                    value: effectRackController.parameterValue(root.effectIndex, effectRackController.rtaPinkNoiseLevelKey())
                    onMoved: v => {
                        renderer.pinkNoiseLevel = v;
                        effectRackController.setParameterValue(root.effectIndex, effectRackController.rtaPinkNoiseLevelKey(), v);
                    }
                    implicitWidth: 80
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Close")
                onClicked: root.close()
                Universal.theme: Universal.Dark
            }
        }
    }
}
