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
    title: applicationService.fmDrumsDeviceName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 700

    // The same short pad labels as Drum Synth V2, whose layout this shares
    readonly property var voiceNames: ["Kick", "Snare", "CHH", "Clap", "OHH", "Lo Tom", "Mid Tom", "Hi Tom", "Crash", "Ride", "Rev Crash", "Rim"]

    function voiceLabel(index) {
        return index < voiceNames.length ? voiceNames[index] : fmDrumsController.voiceName(index);
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: fmDrumsController
            hostDialog: root
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
        onAccepted: () => fmDrumsController.accept()
        onRejected: () => fmDrumsController.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        FmDrumsDialog_Presets {
        }

        // Outside the scrolling area, as on Drum Synth V2: the knobs below are set against it
        DrumPadDialog_WaveformView {
            controller: fmDrumsController
            dialogVisible: root.visible
        }

        ScrollView {
            id: contentScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: contentScrollView.availableWidth
                spacing: 15

                // Three rows rather than one: the pads alone take half the width of the smallest
                // window, and one row of everything would leave the rest of the pad's controls
                // slivers. First the pads, with what most often gets touched beside them.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20

                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        SectionHeading {
                            text: qsTr("Voices")
                        }

                        GridLayout {
                            columns: 4
                            rowSpacing: 8
                            columnSpacing: 8

                            Repeater {
                                model: fmDrumsController.voiceCount()
                                delegate: DrumPad {
                                    required property int index

                                    label: root.voiceLabel(index)
                                    selected: fmDrumsController.selectedVoice === index
                                    onStruck: {
                                        fmDrumsController.selectedVoice = index;
                                        fmDrumsController.playVoice(index);
                                    }
                                    onEffectsRequested: UiService.requestDeviceSubEffectsDialog(fmDrumsController.deviceName(), index, root.voiceLabel(index))
                                }
                            }
                        }
                    }

                    // Vertical separator
                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        color: Theme.separator
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        FmDrumsDialog_Voice {
                            knobColumns: 3
                            voiceName: root.voiceLabel(fmDrumsController.selectedVoice)
                        }

                        SectionHeading {
                            text: qsTr("Mix")
                            Layout.topMargin: 6
                        }

                        GridLayout {
                            columns: 4
                            columnSpacing: 15
                            rowSpacing: 6
                            Layout.fillWidth: true

                            Knob {
                                label: qsTr("Level")
                                mapping: "volume"
                                value: fmDrumsController.voiceLevel
                                onMoved: val => fmDrumsController.voiceLevel = val
                            }
                            Knob {
                                label: qsTr("Pan")
                                mapping: "pan"
                                value: fmDrumsController.voicePan
                                onMoved: val => fmDrumsController.voicePan = val
                            }
                            FilterKnob {
                                // LowPassFilter and HighPassFilter, whose travel starts at 0 Hz
                                startsAtZero: true
                                label: qsTr("LPF")
                                controller: fmDrumsController
                                value: fmDrumsController.voiceLpfCutoff
                                onMoved: val => fmDrumsController.voiceLpfCutoff = val
                            }
                            FilterKnob {
                                startsAtZero: true
                                label: qsTr("HPF")
                                controller: fmDrumsController
                                isHpf: true
                                value: fmDrumsController.voiceHpfCutoff
                                onMoved: val => fmDrumsController.voiceHpfCutoff = val
                            }
                        }
                    }
                }

                // Horizontal separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.separator
                }

                // Then the four operators side by side, and the noise: the whole of the pad's sound
                // in one row, so a modulator and the carrier it drives are read together. Equal
                // widths, by giving every column the same preferred width and letting all of them fill.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15

                    Repeater {
                        model: fmDrumsController.operators
                        delegate: FmDrumsDialog_Operator {
                            required property var modelData
                            op: modelData
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                        }
                    }

                    FmDrumsDialog_Noise {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                    }
                }

                // Horizontal separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.separator
                }

                // Last what changes least: the envelope over the voice and the kit's own settings
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20

                    DrumPadDialog_AmpEg {
                        controller: fmDrumsController
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        voiceName: root.voiceLabel(fmDrumsController.selectedVoice)
                    }

                    // Vertical separator
                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        color: Theme.separator
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        SectionHeading {
                            text: qsTr("Global")
                        }

                        Knob {
                            label: qsTr("Gain")
                            Layout.fillWidth: true
                            mapping: "decibel"
                            mapMin: -30
                            mapMax: 30
                            value: fmDrumsController.gain
                            onMoved: val => fmDrumsController.gain = val
                        }
                        Knob {
                            label: qsTr("Fader")
                            Layout.fillWidth: true
                            mapping: "fader"
                            value: fmDrumsController.volume
                            onMoved: val => fmDrumsController.volume = val
                        }
                        Knob {
                            label: qsTr("Pan")
                            Layout.fillWidth: true
                            mapping: "pan"
                            value: fmDrumsController.pan
                            onMoved: val => fmDrumsController.pan = val
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
                                    currentIndex: fmDrumsController.lpfSlope
                                    onActivated: idx => fmDrumsController.lpfSlope = idx
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
                                    currentIndex: fmDrumsController.hpfSlope
                                    onActivated: idx => fmDrumsController.hpfSlope = idx
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("How steeply every voice's high pass rolls off. The steeper one clears more out of the way at the same cutoff.")
                                }
                            }
                        }
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            activeNotes: fmDrumsController.activeNotes
            onNoteOnRequested: note => fmDrumsController.playNote(note, 1.0)
            onNoteOffRequested: note => fmDrumsController.stopNote(note)
        }
    }
}
