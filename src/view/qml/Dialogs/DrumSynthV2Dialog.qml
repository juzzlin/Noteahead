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
    title: applicationService.drumSynthV2DeviceName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 700

    readonly property var voiceNames: ["Kick", "Snare", "CHH", "Clap", "OHH", "Lo Tom", "Mid Tom", "Hi Tom", "Crash", "Ride", "Rev Crash"]

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: drumSynthV2Controller
            hostDialog: root
            // Reset role: the leftmost group of the box, and like Action it leaves the dialog open
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => drumSynthV2Controller.accept()
        onRejected: () => drumSynthV2Controller.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Above the columns, as the Sampler puts its own waveform above its pads. Outside the
        // scrolling area on purpose: it is what the knobs below are being set against, so it stays
        // put while they scroll.
        DrumSynthV2Dialog_WaveformView {
            dialogVisible: root.visible
        }

        // Everything above the keyboard scrolls together, so a short dialog shortens this area
        // instead of pushing the keyboard out of the dialog
        ScrollView {
            id: contentScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            // One row of columns, as the Sampler lays its pads and settings out: the pads are
            // only four wide, and everything that used to sit under them fits in the space beside
            // them instead.
            RowLayout {
                id: contentRow
                width: contentScrollView.availableWidth
                height: Math.max(implicitHeight, contentScrollView.availableHeight)
                spacing: 20

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                        Label {
                            text: qsTr("Voices")
                            font.bold: true
                            color: themeService.accentColor
                        }

                        GridLayout {
                            columns: 4
                            rowSpacing: 8
                            columnSpacing: 8

                            Repeater {
                                model: root.voiceNames
                                delegate: DrumPad {
                                    // Declared here rather than taken from the context: a delegate
                                    // whose root has required properties is handed its model data
                                    // by name, and nothing is injected implicitly.
                                    required property int index
                                    required property string modelData

                                    label: modelData
                                    selected: drumSynthV2Controller.selectedVoice === index
                                    onStruck: {
                                        drumSynthV2Controller.selectedVoice = index;
                                        drumSynthV2Controller.playVoice(index);
                                    }
                                    onEffectsRequested: UiService.requestDeviceSubEffectsDialog(drumSynthV2Controller.deviceName(), index, root.voiceNames[index])
                                }
                            }
                        }
                    }

                // Vertical separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                // Two columns rather than one long row: the controls then stand beside the pads in
                // the space the row used to leave empty, and the dialog stops being taller than it
                // needs to be. The Sampler lays its pad settings out the same way.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.28
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                    Label {
                        text: qsTr("Voice Settings") + " (" + root.voiceNames[drumSynthV2Controller.selectedVoice] + ")"
                        font.bold: true
                        color: themeService.accentColor
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 15
                        rowSpacing: 6
                        Layout.fillWidth: true


                        Knob {
                            label: qsTr("Level")
                            mapping: "volume"
                            value: drumSynthV2Controller.voiceLevel
                            onMoved: (val) => drumSynthV2Controller.voiceLevel = val
                        }
                        Knob {
                            label: qsTr("Pan")
                            mapping: "pan"
                            value: drumSynthV2Controller.voicePan
                            onMoved: (val) => drumSynthV2Controller.voicePan = val
                        }
                        FilterKnob {
                            label: qsTr("LPF")
                            controller: drumSynthV2Controller
                            value: drumSynthV2Controller.voiceLpfCutoff
                            onMoved: (val) => drumSynthV2Controller.voiceLpfCutoff = val
                        }
                        FilterKnob {
                            label: qsTr("HPF")
                            controller: drumSynthV2Controller
                            isHpf: true
                            value: drumSynthV2Controller.voiceHpfCutoff
                            onMoved: (val) => drumSynthV2Controller.voiceHpfCutoff = val
                        }
                        Knob {
                            label: qsTr("Tune")
                            value: drumSynthV2Controller.voiceTune
                            onMoved: (val) => drumSynthV2Controller.voiceTune = val
                        }
                        Knob {
                            label: qsTr("Decay")
                            value: drumSynthV2Controller.voiceDecay
                            onMoved: (val) => drumSynthV2Controller.voiceDecay = val
                        }

                        // Voice Specific
                        Knob {
                            visible: drumSynthV2Controller.isKick
                            label: qsTr("Attack")
                            mapping: "cubic"
                            value: drumSynthV2Controller.kickAttack
                            onMoved: (val) => drumSynthV2Controller.kickAttack = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isKick
                            label: qsTr("C.Tune")
                            value: drumSynthV2Controller.kickClickTune
                            onMoved: (val) => drumSynthV2Controller.kickClickTune = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isKick
                            label: qsTr("P.Depth")
                            value: drumSynthV2Controller.kickPitchDepth
                            onMoved: (val) => drumSynthV2Controller.kickPitchDepth = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isKick
                            label: qsTr("P.Decay")
                            value: drumSynthV2Controller.kickPitchDecay
                            onMoved: (val) => drumSynthV2Controller.kickPitchDecay = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isSnare
                            label: qsTr("Snappy")
                            value: drumSynthV2Controller.snareSnappy
                            onMoved: (val) => drumSynthV2Controller.snareSnappy = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isSnare
                            label: qsTr("Tone")
                            value: drumSynthV2Controller.snareTone
                            onMoved: (val) => drumSynthV2Controller.snareTone = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isTom
                            label: qsTr("P.Depth")
                            value: drumSynthV2Controller.tomPitchDepth
                            onMoved: (val) => drumSynthV2Controller.tomPitchDepth = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.isTom
                            label: qsTr("P.Decay")
                            value: drumSynthV2Controller.tomPitchDecay
                            onMoved: (val) => drumSynthV2Controller.tomPitchDecay = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.hasAttack && !drumSynthV2Controller.isKick
                            label: qsTr("Attack")
                            value: drumSynthV2Controller.voiceAttack
                            onMoved: (val) => drumSynthV2Controller.voiceAttack = val
                        }
                        Knob {
                            visible: drumSynthV2Controller.hasResonance
                            label: qsTr("Reso")
                            value: drumSynthV2Controller.voiceResonance
                            onMoved: (val) => drumSynthV2Controller.voiceResonance = val
                        }
                    }
                }

                // Vertical separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                DrumSynthV2Dialog_AmpEg {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.2
                    Layout.alignment: Qt.AlignTop
                    voiceName: root.voiceNames[drumSynthV2Controller.selectedVoice]
                }
                // Vertical separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                // Last of the columns, as the Sampler puts its own global section last.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.2
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                        Label {
                            text: qsTr("Global")
                            font.bold: true
                            color: themeService.accentColor
                        }

                        Knob {
                            label: qsTr("Gain")
                            Layout.fillWidth: true
                            mapping: "decibel"
                            mapMin: -30
                            mapMax: 30
                            value: drumSynthV2Controller.gain
                            onMoved: (val) => drumSynthV2Controller.gain = val
                        }
                        Knob {
                            label: qsTr("Fader")
                            Layout.fillWidth: true
                            mapping: "fader"
                            value: drumSynthV2Controller.volume
                            onMoved: (val) => drumSynthV2Controller.volume = val
                        }
                        Knob {
                            label: qsTr("Pan")
                            Layout.fillWidth: true
                            mapping: "pan"
                            value: drumSynthV2Controller.pan
                            onMoved: (val) => drumSynthV2Controller.pan = val
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("LPF Slope")
                                }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["12 dB/oct", "24 dB/oct"]
                                    currentIndex: drumSynthV2Controller.lpfSlope
                                    onActivated: idx => drumSynthV2Controller.lpfSlope = idx
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("How steeply every voice's low pass rolls off. The steeper one clears more out of the way at the same cutoff.")
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("HPF Slope")
                                }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["12 dB/oct", "24 dB/oct"]
                                    currentIndex: drumSynthV2Controller.hpfSlope
                                    onActivated: idx => drumSynthV2Controller.hpfSlope = idx
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("How steeply every voice's high pass rolls off. The steeper one clears more out of the way at the same cutoff.")
                                }
                            }
                        }
                    }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            activeNotes: drumSynthV2Controller.activeNotes
            onNoteOnRequested: (note) => drumSynthV2Controller.playNote(note, 1.0)
            onNoteOffRequested: (note) => drumSynthV2Controller.stopNote(note)
        }
    }
}
