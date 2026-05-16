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

Dialog {
    id: root
    title: applicationService.drumSynthDeviceName
    modal: true
    focus: true
    width: 800
    height: 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Global Controls
        GroupBox {
            title: qsTr("Global")
            Layout.fillWidth: true
            RowLayout {
                spacing: 20
                Knob {
                    label: qsTr("Volume")
                    value: drumSynthController.volume
                    onMoved: (val) => drumSynthController.volume = val
                }
                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    value: drumSynthController.gain
                    onMoved: (val) => drumSynthController.gain = val
                }
                Knob {
                    label: qsTr("Pan")
                    mapping: "pan"
                    value: drumSynthController.pan
                    onMoved: (val) => drumSynthController.pan = val
                }
            }
        }

        // Voice Grid
        GroupBox {
            title: qsTr("Voices")
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            GridLayout {
                columns: 4
                anchors.fill: parent
                rowSpacing: 10
                columnSpacing: 10

                Repeater {
                    model: ["Kick", "Snare", "CHH", "Clap", "OHH", "Lo Tom", "Mid Tom", "Hi Tom", "Crash", "Ride", "Rev Crash"]
                    delegate: Button {
                        text: modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        highlighted: drumSynthController.selectedVoice === index
                        onClicked: {
                            drumSynthController.selectedVoice = index
                            drumSynthController.playVoice(index)
                        }
                    }
                }
            }
        }

        // Voice Settings
        GroupBox {
            title: qsTr("Voice Settings") + " (" + ["Kick", "Snare", "CHH", "Clap", "OHH", "Lo Tom", "Mid Tom", "Hi Tom", "Crash", "Ride", "Rev Crash"][drumSynthController.selectedVoice] + ")"
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            
            ScrollView {
                anchors.fill: parent
                contentWidth: settingsRow.implicitWidth
                clip: true

                RowLayout {
                    id: settingsRow
                    spacing: 15
                    
                    Knob {
                        label: qsTr("Level")
                        value: drumSynthController.voiceLevel
                        onMoved: (val) => drumSynthController.voiceLevel = val
                    }
                    Knob {
                        label: qsTr("Pan")
                        mapping: "pan"
                        value: drumSynthController.voicePan
                        onMoved: (val) => drumSynthController.voicePan = val
                    }
                    FilterKnob {
                        label: qsTr("LPF")
                        controller: drumSynthController
                        value: drumSynthController.voiceLpfCutoff
                        onMoved: (val) => drumSynthController.voiceLpfCutoff = val
                    }
                    FilterKnob {
                        label: qsTr("HPF")
                        controller: drumSynthController
                        isHpf: true
                        value: drumSynthController.voiceHpfCutoff
                        onMoved: (val) => drumSynthController.voiceHpfCutoff = val
                    }
                    Knob {
                        label: qsTr("Tune")
                        value: drumSynthController.voiceTune
                        onMoved: (val) => drumSynthController.voiceTune = val
                    }
                    Knob {
                        label: qsTr("Decay")
                        value: drumSynthController.voiceDecay
                        onMoved: (val) => drumSynthController.voiceDecay = val
                    }

                    // Voice Specific
                    Knob {
                        visible: drumSynthController.isKick
                        label: qsTr("Attack")
                        mapping: "cubic"
                        value: drumSynthController.kickAttack
                        onMoved: (val) => drumSynthController.kickAttack = val
                    }
                    Knob {
                        visible: drumSynthController.isKick
                        label: qsTr("C.Tune")
                        value: drumSynthController.kickClickTune
                        onMoved: (val) => drumSynthController.kickClickTune = val
                    }
                    Knob {
                        visible: drumSynthController.isKick
                        label: qsTr("P.Depth")
                        value: drumSynthController.kickPitchDepth
                        onMoved: (val) => drumSynthController.kickPitchDepth = val
                    }
                    Knob {
                        visible: drumSynthController.isKick
                        label: qsTr("P.Decay")
                        value: drumSynthController.kickPitchDecay
                        onMoved: (val) => drumSynthController.kickPitchDecay = val
                    }
                    Knob {
                        visible: drumSynthController.isSnare
                        label: qsTr("Snappy")
                        value: drumSynthController.snareSnappy
                        onMoved: (val) => drumSynthController.snareSnappy = val
                    }
                    Knob {
                        visible: drumSynthController.isSnare
                        label: qsTr("Tone")
                        value: drumSynthController.snareTone
                        onMoved: (val) => drumSynthController.snareTone = val
                    }
                    Knob {
                        visible: drumSynthController.isTom
                        label: qsTr("P.Depth")
                        value: drumSynthController.tomPitchDepth
                        onMoved: (val) => drumSynthController.tomPitchDepth = val
                    }
                    Knob {
                        visible: drumSynthController.isTom
                        label: qsTr("P.Decay")
                        value: drumSynthController.tomPitchDecay
                        onMoved: (val) => drumSynthController.tomPitchDecay = val
                    }
                    Knob {
                        visible: drumSynthController.hasAttack && !drumSynthController.isKick
                        label: qsTr("Attack")
                        value: drumSynthController.voiceAttack
                        onMoved: (val) => drumSynthController.voiceAttack = val
                    }
                    Knob {
                        visible: drumSynthController.hasResonance
                        label: qsTr("Reso")
                        value: drumSynthController.voiceResonance
                        onMoved: (val) => drumSynthController.voiceResonance = val
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: (note) => drumSynthController.playNote(note, 1.0)
            onNoteOffRequested: (note) => drumSynthController.stopNote(note)
        }
    }
}
