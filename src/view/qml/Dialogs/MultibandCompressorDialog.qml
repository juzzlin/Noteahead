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
    property var bandReductionDb: [0.0, 0.0, 0.0]
    title: "<strong>" + qsTr("Multiband Compressor (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Knob {
                label: qsTr("Low/Mid")
                suffix: "Hz"
                mapping: "logFrequency"
                mapMin: 20
                mapMax: 20000
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.multibandCompressorCrossoverFreqKey(0)) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.multibandCompressorCrossoverFreqKey(0), v / 1000)
            }

            Knob {
                label: qsTr("Mid/High")
                suffix: "Hz"
                mapping: "logFrequency"
                mapMin: 20
                mapMax: 20000
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.multibandCompressorCrossoverFreqKey(1)) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.multibandCompressorCrossoverFreqKey(1), v / 1000)
            }

            Knob {
                label: qsTr("Output")
                suffix: "dB"
                from: -12
                to: 12
                value: {
                    effectRackController.revision;
                    return -12 + effectRackController.parameterValue(root.effectIndex, effectRackController.multibandCompressorGainKey()) * 24;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.multibandCompressorGainKey(), (v + 12) / 24)
            }

            ColumnLayout {
                spacing: 5
                Label {
                    text: qsTr("Detector")
                    font.bold: true
                    font.pixelSize: 11
                    color: "#aaa"
                }
                ComboBox {
                    implicitWidth: 110
                    model: [qsTr("Peak"), qsTr("RMS")]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.multibandCompressorModeKey());
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.multibandCompressorModeKey(), index)
                }
            }

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Label {
                    text: qsTr("Side Chain Source")
                    font.bold: true
                    font.pixelSize: 11
                    color: "#aaa"
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: {
                        var items = [qsTr("None")];
                        for (var i = 0; i < deviceRackController.deviceCount; i++) {
                            items.push(qsTr("Device %1").arg(i + 1));
                        }
                        return items;
                    }
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.multibandCompressorSideChainSourceDeviceKey()) + 1;
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.multibandCompressorSideChainSourceDeviceKey(), index - 1)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            Repeater {
                model: [qsTr("Low"), qsTr("Mid"), qsTr("High")]
                delegate: BandSettings {
                    bandIndex: index
                    bandName: modelData
                    effectIndex: root.effectIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

    component BandSettings: RowLayout {
        id: band
        property int bandIndex: -1
        property int effectIndex: -1
        property string bandName: ""
        spacing: 12

        // Every band but the first is fenced off from the one before it.
        Rectangle {
            width: 1
            color: "#333"
            visible: band.bandIndex > 0
            Layout.fillHeight: true
            Layout.rightMargin: 8
        }

        ColumnLayout {
            spacing: 12
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                text: "<strong>" + band.bandName + "</strong>"
                font.pointSize: 12
                color: themeService.accentColor
                Layout.alignment: Qt.AlignHCenter
            }

            GridLayout {
                columns: 2
                columnSpacing: 15
                rowSpacing: 12
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Threshold")
                    suffix: "dB"
                    from: -60
                    to: 0
                    value: {
                        effectRackController.revision;
                        return -60 + effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorThresholdKey(band.bandIndex)) * 60;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorThresholdKey(band.bandIndex), (v + 60) / 60)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Ratio")
                    suffix: ":1"
                    from: 1
                    to: 20
                    value: {
                        effectRackController.revision;
                        return 1 + effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorRatioKey(band.bandIndex)) * 19;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorRatioKey(band.bandIndex), (v - 1) / 19)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Attack")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 0.1
                    mapMax: 500
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorAttackKey(band.bandIndex)) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorAttackKey(band.bandIndex), v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Release")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 1
                    mapMax: 2000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorReleaseKey(band.bandIndex)) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorReleaseKey(band.bandIndex), v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Knee")
                    suffix: "dB"
                    from: 0
                    to: 24
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorKneeKey(band.bandIndex)) * 24;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorKneeKey(band.bandIndex), v / 24)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Makeup")
                    suffix: "dB"
                    from: -12
                    to: 12
                    value: {
                        effectRackController.revision;
                        return -12 + effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorMakeupKey(band.bandIndex)) * 24;
                    }
                    onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorMakeupKey(band.bandIndex), (v + 12) / 24)
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter

                Switch {
                    text: qsTr("Bypass")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorBypassKey(band.bandIndex)) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorBypassKey(band.bandIndex), checked ? 1 : 0)
                }

                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(band.effectIndex, effectRackController.multibandCompressorSoloKey(band.bandIndex)) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(band.effectIndex, effectRackController.multibandCompressorSoloKey(band.bandIndex), checked ? 1 : 0)
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }

        Rectangle {
            width: 1
            color: "#333"
            Layout.fillHeight: true
        }

        ColumnLayout {
            spacing: 4
            Layout.fillHeight: true
            // Fixed, so that nothing the meter displays can feed back into the band's width.
            Layout.preferredWidth: 44
            Layout.fillWidth: false

            Label {
                text: qsTr("GR")
                font.bold: true
                font.pixelSize: 11
                color: "#aaa"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                width: 24
                color: "#111"
                border.color: "#333"
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    width: parent.width - 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: Math.min(parent.height - 4, (Math.abs(root.bandReductionDb[band.bandIndex]) / 30.0) * (parent.height - 4))
                    color: themeService.accentColor
                }

                // Tick marks every 5 dB down to -30 dB.
                Item {
                    anchors.fill: parent
                    anchors.margins: 2
                    Repeater {
                        model: 7
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
                // Padded to the width of the widest reading with U+2007 FIGURE SPACE, which is
                // exactly as wide as a digit. Without it the minus sign coming and going as the
                // band crosses into reduction would jog the number sideways thirty times a second.
                text: root.bandReductionDb[band.bandIndex].toFixed(1).padStart(5, "\u2007")
                color: themeService.accentColor
                font.family: "Monospace"
                font.pixelSize: 11
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.bandReductionDb = [effectRackController.multibandCompressorBandReductionDb(root.effectIndex, 0), effectRackController.multibandCompressorBandReductionDb(root.effectIndex, 1), effectRackController.multibandCompressorBandReductionDb(root.effectIndex, 2)];
        }
    }
}
