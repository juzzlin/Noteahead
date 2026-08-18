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
    property var bandCorrelation: [1.0, 1.0, 1.0]
    title: "<strong>" + qsTr("Stereo Widener (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        // Neither bar is turned off: whatever the window is reduced to, everything stays reachable.
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            // Fills the dialog when there is room and keeps its own width when there is not, which
            // is what leaves the scroll view something to scroll.
            width: Math.max(implicitWidth, scrollView.availableWidth)
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
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerCrossoverFreqKey(0)) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoWidenerCrossoverFreqKey(0), v / 1000)
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
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerCrossoverFreqKey(1)) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoWidenerCrossoverFreqKey(1), v / 1000)
                }

                Knob {
                    label: qsTr("Output")
                    suffix: "dB"
                    from: -12
                    to: 12
                    value: {
                        effectRackController.revision;
                        return (effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerGainKey()) - 0.5) * 24;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoWidenerGainKey(), v / 24 + 0.5)
                }

                Rectangle {
                    width: 1
                    color: "#333"
                    Layout.fillHeight: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                }

                Knob {
                    label: qsTr("Mono Below")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 20
                    mapMax: 300
                    from: 0
                    to: 1000
                    enabled: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerMonoBassKey()) > 0.5;
                    }
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerMonoFreqKey()) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoWidenerMonoFreqKey(), v / 1000)
                }

                Switch {
                    text: qsTr("Mono Bass")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoWidenerMonoBassKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoWidenerMonoBassKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Centre everything below the corner, after the bands, whatever their width controls did")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#333"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                Repeater {
                    model: [qsTr("Low"), qsTr("Mid"), qsTr("High")]
                    delegate: BandSettings {
                        bandIndex: index
                        bandName: modelData
                        effectIndex: root.effectIndex
                        Layout.fillWidth: true
                    }
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

            Knob {
                label: qsTr("Width")
                suffix: "%"
                mapping: "percentage"
                from: 0
                to: 200
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(band.effectIndex, effectRackController.stereoWidenerWidthKey(band.bandIndex)) * 200;
                }
                onMoved: v => effectRackController.setParameterValue(band.effectIndex, effectRackController.stereoWidenerWidthKey(band.bandIndex), v / 200)
                Layout.fillWidth: true
            }

            Switch {
                text: qsTr("Solo")
                checked: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(band.effectIndex, effectRackController.stereoWidenerSoloKey(band.bandIndex)) > 0.5;
                }
                onToggled: effectRackController.setParameterValue(band.effectIndex, effectRackController.stereoWidenerSoloKey(band.bandIndex), checked ? 1 : 0)
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("Correlation")
                font.bold: true
                font.pixelSize: 11
                color: "#aaa"
                Layout.alignment: Qt.AlignHCenter
            }

            // Runs from -1 on the left to +1 on the right. The bar grows out of the centre, so how
            // far the band has been taken from mono reads as distance from the middle, and anything
            // sustained to the left of it will cancel when the mix is summed to mono.
            Rectangle {
                height: 18
                color: "#111"
                border.color: "#333"
                Layout.fillWidth: true

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height - 4
                    width: Math.abs(root.bandCorrelation[band.bandIndex]) * (parent.width - 4) / 2
                    x: root.bandCorrelation[band.bandIndex] >= 0 ? parent.width / 2 : parent.width / 2 - width
                    color: root.bandCorrelation[band.bandIndex] >= 0 ? themeService.accentColor : "#c0392b"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 1
                    height: parent.height
                    color: "#777"
                }
            }

            Label {
                // Padded to the width of the widest reading with U+2007 FIGURE SPACE, which is
                // exactly as wide as a digit. Without it the minus sign coming and going would jog
                // the number sideways thirty times a second.
                text: root.bandCorrelation[band.bandIndex].toFixed(2).padStart(5, " ")
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
            root.bandCorrelation = [effectRackController.stereoWidenerBandCorrelation(root.effectIndex, 0), effectRackController.stereoWidenerBandCorrelation(root.effectIndex, 1), effectRackController.stereoWidenerBandCorrelation(root.effectIndex, 2)];
        }
    }
}
