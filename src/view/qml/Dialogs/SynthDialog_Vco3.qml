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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    Label {
        text: qsTr("VCO 3")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    // The bar sits under the pages, as the dialog's own tabs do. The stack is held to the taller of
    // the two pages so the bar keeps still while the page changes.
    StackLayout {
        id: vcoPages
        currentIndex: vcoTabBar.currentIndex
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(shapePage.implicitHeight, filterPage.implicitHeight)

        ColumnLayout {
            id: shapePage
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: Constants.dropDownBottomMargin
                ComboBox {
                    model: synthController.vcoWaveformNames
                    currentIndex: synthController.vco3Waveform
                    onActivated: i => synthController.vco3Waveform = i
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: synthController.octaveNames
                    currentIndex: synthController.vco3Octave + 2
                    onActivated: i => synthController.vco3Octave = i - 2
                    Layout.fillWidth: true
                }
            }
            Knob {
                label: qsTr("Pitch")
                mapping: "cubicCentered"
                mapMin: -2400
                mapMax: 2400
                suffix: "c"
                value: synthController.vco3Pitch
                onMoved: v => synthController.vco3Pitch = v
                Layout.fillWidth: true
            }
            Knob {
                label: qsTr("Shape")
                value: synthController.vco3Shape
                onMoved: v => synthController.vco3Shape = v
                Layout.fillWidth: true
            }
            Knob {
                // Only the pulse has edges to round; a saw's ramp is the waveform itself.
                label: qsTr("Roundness")
                enabled: synthController.vco3Waveform === synthController.squareWaveformIndex
                value: synthController.vco3Roundness
                onMoved: v => synthController.vco3Roundness = v
                Layout.fillWidth: true
            }
            Knob {
                label: qsTr("Level")
                value: synthController.mixVco3
                onMoved: v => synthController.mixVco3 = v
                Layout.fillWidth: true
            }
            CheckBox {
                text: qsTr("Hard Sync to VCO2")
                checked: synthController.vco3Sync
                onToggled: synthController.vco3Sync = checked
            }
        }

        ColumnLayout {
            id: filterPage
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Layout.bottomMargin: Constants.dropDownBottomMargin

                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("LPF Slope")
                    }
                    ComboBox {
                        model: ["12 dB/oct", "24 dB/oct"]
                        currentIndex: synthController.vco3LpfSlope
                        onActivated: idx => synthController.vco3LpfSlope = idx
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("HPF Slope")
                    }
                    ComboBox {
                        model: ["12 dB/oct", "24 dB/oct"]
                        currentIndex: synthController.vco3HpfSlope
                        onActivated: idx => synthController.vco3HpfSlope = idx
                        Layout.fillWidth: true
                    }
                }
            }

            FilterKnob {
                label: qsTr("LPF Cutoff")
                controller: synthController
                value: synthController.vco3LpfCutoff
                onMoved: v => synthController.vco3LpfCutoff = v
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Rolls the top off this oscillator alone, before it reaches the mix and the voice's own filter.")
                Layout.fillWidth: true
            }
            Knob {
                label: qsTr("LPF Resonance")
                value: synthController.vco3LpfResonance
                onMoved: v => synthController.vco3LpfResonance = v
                Layout.fillWidth: true
            }
            FilterKnob {
                label: qsTr("HPF Cutoff")
                controller: synthController
                value: synthController.vco3HpfCutoff
                isHpf: true
                onMoved: v => synthController.vco3HpfCutoff = v
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Thins out this oscillator alone, which is how one is made to sit under another rather than against it.")
                Layout.fillWidth: true
            }
        }
    }

    TabBar {
        id: vcoTabBar
        Layout.fillWidth: true
        TabButton {
            text: qsTr("Shape")
        }
        TabButton {
            // A tab hides what is behind it, so the label says when this oscillator is being
            // filtered at all -- by a control moved off its end, or by something sweeping one.
            text: qsTr("Filter")
            font.bold: synthController.vco3FilterEngaged
        }
    }
}
