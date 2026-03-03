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
    title: drumSynth808Controller.displayName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 700

    // Short labels for the pads, which are not the device's own names: a pad is too narrow for
    // "Reverse Cymbal". Only labels -- how many pads there are comes from the device, so a voice
    // added there cannot go missing here for want of an entry in this list. The second list is what
    // a pad plays with its channel switched, as the RD-8 labels its switches.
    readonly property var voiceNames: ["Kick", "Snare", "CHH", "Clap", "OHH", "Lo Tom", "Mid Tom", "Hi Tom", "Cymbal", "Cowbell", "Rev Cymbal", "Rim"]
    readonly property var switchedNames: ["", "", "", "Maracas", "", "Lo Conga", "Mid Conga", "Hi Conga", "", "", "", "Claves"]

    function voiceLabel(index) {
        drumSynth808Controller.knobRevision;
        if (index < switchedNames.length && switchedNames[index] !== "" && drumSynth808Controller.voiceSwitched(index)) {
            return switchedNames[index];
        }
        return index < voiceNames.length ? voiceNames[index] : drumSynth808Controller.voiceName(index);
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
            controller: drumSynth808Controller
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
        onAccepted: () => drumSynth808Controller.accept()
        onRejected: () => drumSynth808Controller.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        DrumSynth808Dialog_Presets {
        }

        // Above the columns, as the Sampler puts its own waveform above its pads. Outside the
        // scrolling area on purpose: it is what the knobs below are being set against, so it stays
        // put while they scroll.
        DrumPadDialog_WaveformView {
            controller: drumSynth808Controller
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

                        SectionHeading {
                            text: qsTr("Voices")
                        }

                        GridLayout {
                            columns: 4
                            rowSpacing: 8
                            columnSpacing: 8

                            Repeater {
                                model: drumSynth808Controller.voiceCount()
                                delegate: DrumPad {
                                    // Declared here rather than taken from the context: a delegate
                                    // whose root has required properties is handed its model data
                                    // by name, and nothing is injected implicitly.
                                    required property int index

                                    label: root.voiceLabel(index)
                                    selected: drumSynth808Controller.selectedVoice === index
                                    onStruck: {
                                        drumSynth808Controller.selectedVoice = index;
                                        drumSynth808Controller.playVoice(index);
                                    }
                                    onEffectsRequested: UiService.requestDeviceSubEffectsDialog(drumSynth808Controller.deviceName(), index, root.voiceLabel(index))
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

                // Two columns rather than one long row: the controls then stand beside the pads in
                // the space the row used to leave empty, and the dialog stops being taller than it
                // needs to be. The Sampler lays its pad settings out the same way.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.28
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                    SectionHeading {
                        text: qsTr("Voice Settings") + " (" + root.voiceLabel(drumSynth808Controller.selectedVoice) + ")"
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 15
                        rowSpacing: 6
                        Layout.fillWidth: true

                        Knob {
                            label: qsTr("Level")
                            mapping: "fader"
                            value: drumSynth808Controller.voiceLevel
                            onMoved: (val) => drumSynth808Controller.voiceLevel = val
                        }
                        Knob {
                            label: qsTr("Pan")
                            mapping: "pan"
                            value: drumSynth808Controller.voicePan
                            onMoved: (val) => drumSynth808Controller.voicePan = val
                        }
                        FilterKnob {
                            // LowPassFilter and HighPassFilter, whose travel starts at 0 Hz
                            startsAtZero: true
                            label: qsTr("LPF")
                            controller: drumSynth808Controller
                            value: drumSynth808Controller.voiceLpfCutoff
                            onMoved: (val) => drumSynth808Controller.voiceLpfCutoff = val
                        }
                        FilterKnob {
                            // LowPassFilter and HighPassFilter, whose travel starts at 0 Hz
                            startsAtZero: true
                            label: qsTr("HPF")
                            controller: drumSynth808Controller
                            isHpf: true
                            value: drumSynth808Controller.voiceHpfCutoff
                            onMoved: (val) => drumSynth808Controller.voiceHpfCutoff = val
                        }
                        // The drum's own knobs, as the RD-8's panel has them: each drum has
                        // different ones, so they come from the controller as a list. The
                        // channel's switch comes last, as a check box named after the other voice.
                        Repeater {
                            model: drumSynth808Controller.voiceKnobs
                            delegate: Knob {
                                required property var modelData

                                visible: !modelData.isSwitch
                                label: modelData.label
                                value: {
                                    drumSynth808Controller.knobRevision;
                                    return drumSynth808Controller.knob(modelData.key);
                                }
                                onMoved: (val) => drumSynth808Controller.setKnob(modelData.key, val)
                            }
                        }
                        Repeater {
                            model: drumSynth808Controller.voiceKnobs
                            delegate: CheckBox {
                                required property var modelData

                                visible: modelData.isSwitch
                                Layout.columnSpan: 2
                                text: modelData.label
                                checked: {
                                    drumSynth808Controller.knobRevision;
                                    return drumSynth808Controller.voiceSwitched(drumSynth808Controller.selectedVoice);
                                }
                                onToggled: drumSynth808Controller.setVoiceSwitched(checked)
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("The RD-8's switch: plays the channel's other voice")
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

                DrumPadDialog_AmpEg {
                    controller: drumSynth808Controller
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.2
                    Layout.alignment: Qt.AlignTop
                    voiceName: root.voiceLabel(drumSynth808Controller.selectedVoice)
                }
                // Vertical separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: Theme.separator
                }

                // Last of the columns, as the Sampler puts its own global section last.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentRow.width * 0.2
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
                            value: drumSynth808Controller.gain
                            onMoved: (val) => drumSynth808Controller.gain = val
                        }
                        Knob {
                            label: qsTr("Fader")
                            Layout.fillWidth: true
                            mapping: "fader"
                            value: drumSynth808Controller.volume
                            onMoved: (val) => drumSynth808Controller.volume = val
                        }
                        Knob {
                            label: qsTr("Pan")
                            Layout.fillWidth: true
                            mapping: "pan"
                            value: drumSynth808Controller.pan
                            onMoved: (val) => drumSynth808Controller.pan = val
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
                                    currentIndex: drumSynth808Controller.lpfSlope
                                    onActivated: idx => drumSynth808Controller.lpfSlope = idx
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
                                    currentIndex: drumSynth808Controller.hpfSlope
                                    onActivated: idx => drumSynth808Controller.hpfSlope = idx
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
            activeNotes: drumSynth808Controller.activeNotes
            onNoteOnRequested: (note) => drumSynth808Controller.playNote(note, 1.0)
            onNoteOffRequested: (note) => drumSynth808Controller.stopNote(note)
        }
    }
}
