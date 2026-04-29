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

Dialog {
    id: root
    title: qsTr("Notealogue Synthesizer")
    modal: true
    focus: true
    width: 1000
    height: 800

    Universal.accent: themeService.accentColor

    onAboutToShow: () => {
        synthController.requestSettings();
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => {
            synthController.accept();
        }
        onRejected: () => {
            synthController.reject();
        }
    }

    component Knob : ColumnLayout {
        id: knobRoot
        property string label: ""
        property real value: 0
        property real from: 0
        property real to: 100
        property string suffix: "%"
        signal moved(real val)

        spacing: 2
        Label {
            text: knobRoot.label + " (" + Math.round(knobRoot.value) + knobRoot.suffix + ")"
            font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }
        Slider {
            from: knobRoot.from
            to: knobRoot.to
            value: knobRoot.value
            stepSize: 1
            Layout.fillWidth: true
            onMoved: () => knobRoot.moved(value)
        }
    }

    component SectionTitle : Label {
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        // Header and Presets
        RowLayout {
            Layout.fillWidth: true
            SectionTitle { text: qsTr("Notealogue"); Layout.fillWidth: true }
            
            Label { text: qsTr("Preset:") }
            ComboBox {
                id: presetCombo
                model: ["Init", "Fat Bass", "Soft Pad", "Sync Lead", "Bright Pluck", "Sub Bass", "Strings", "Organ", "Bell", "Classic Poly"]
                onActivated: (index) => synthController.loadPreset(index)
            }

            Button {
                text: qsTr("Reset")
                onClicked: synthController.reset()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridLayout {
                columns: 4
                columnSpacing: 20
                rowSpacing: 5
                width: parent.width - 20

                // Row 1: Titles
                SectionTitle { text: "VCO 1" }
                SectionTitle { text: "VCO 2" }
                SectionTitle { text: "Mixer" }
                SectionTitle { text: "Filter" }

                // Row 2: VCO/Mixer/Filter Controls
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox { model: ["Triangle", "Saw", "Pulse"]; currentIndex: synthController.vco1Waveform; onActivated: (i) => synthController.vco1Waveform = i; Layout.fillWidth: true }
                        ComboBox { model: ["16'", "8'", "4'", "2'"]; currentIndex: synthController.vco1Octave + 1; onActivated: (i) => synthController.vco1Octave = i - 1; Layout.fillWidth: true }
                    }
                    Knob { label: qsTr("Pitch"); from: -100; to: 100; suffix: "c"; value: synthController.vco1Pitch; onMoved: (v) => synthController.vco1Pitch = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Shape"); from: 0; to: 100; value: synthController.vco1Shape; onMoved: (v) => synthController.vco1Shape = v; Layout.fillWidth: true }
                    CheckBox { text: qsTr("Phase Sync"); checked: synthController.vco1Sync; onToggled: synthController.vco1Sync = checked }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox { model: ["Triangle", "Saw", "Pulse"]; currentIndex: synthController.vco2Waveform; onActivated: (i) => synthController.vco2Waveform = i; Layout.fillWidth: true }
                        ComboBox { model: ["16'", "8'", "4'", "2'"]; currentIndex: synthController.vco2Octave + 1; onActivated: (i) => synthController.vco2Octave = i - 1; Layout.fillWidth: true }
                    }
                    Knob { label: qsTr("Pitch"); from: -100; to: 100; suffix: "c"; value: synthController.vco2Pitch; onMoved: (v) => synthController.vco2Pitch = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Shape"); from: 0; to: 100; value: synthController.vco2Shape; onMoved: (v) => synthController.vco2Shape = v; Layout.fillWidth: true }
                    CheckBox { text: qsTr("Hard Sync to VCO1"); checked: synthController.vco2Sync; onToggled: synthController.vco2Sync = checked }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Knob { label: qsTr("VCO 1 Level"); value: synthController.mixVco1; onMoved: (v) => synthController.mixVco1 = v; Layout.fillWidth: true }
                    Knob { label: qsTr("VCO 2 Level"); value: synthController.mixVco2; onMoved: (v) => synthController.mixVco2 = v; Layout.fillWidth: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Knob { label: qsTr("LPF Cutoff"); value: synthController.lpfCutoff; onMoved: (v) => synthController.lpfCutoff = v; Layout.fillWidth: true }
                    Knob { label: qsTr("LPF Resonance"); value: synthController.lpfResonance; onMoved: (v) => synthController.lpfResonance = v; Layout.fillWidth: true }
                    Knob { label: qsTr("HPF Cutoff"); value: synthController.hpfCutoff; onMoved: (v) => synthController.hpfCutoff = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Key Track"); value: synthController.filterKeyTrack; onMoved: (v) => synthController.filterKeyTrack = v; Layout.fillWidth: true }
                }

                // Spacing Row
                Item { Layout.fillWidth: true; Layout.preferredHeight: 10; Layout.columnSpan: 4 }

                // Row 3: Titles
                SectionTitle { text: "Voice / Global" }
                SectionTitle { text: "Amp EG (ADSR)" }
                SectionTitle { text: "Mod EG (AD)" }
                SectionTitle { text: "Delay Effect" }

                // Row 4: Controls
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox { model: ["Poly", "Unison"]; currentIndex: synthController.voiceMode; onActivated: (i) => synthController.voiceMode = i; Layout.fillWidth: true }
                    }
                    Knob { label: qsTr("Voice Depth"); value: synthController.voiceDepth; onMoved: (v) => synthController.voiceDepth = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Portamento"); value: synthController.portamento; onMoved: (v) => synthController.portamento = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Pan Spread"); value: synthController.panSpread; onMoved: (v) => synthController.panSpread = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Master Volume"); value: synthController.masterVolume; onMoved: (v) => synthController.masterVolume = v; Layout.fillWidth: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Knob { label: qsTr("Attack"); value: synthController.ampAttack; onMoved: (v) => synthController.ampAttack = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Decay"); value: synthController.ampDecay; onMoved: (v) => synthController.ampDecay = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Sustain"); value: synthController.ampSustain; onMoved: (v) => synthController.ampSustain = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Release"); value: synthController.ampRelease; onMoved: (v) => synthController.ampRelease = v; Layout.fillWidth: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox { model: ["Pitch 1", "Pitch 2", "Cutoff"]; currentIndex: synthController.modTarget; onActivated: (i) => synthController.modTarget = i; Layout.fillWidth: true }
                    }
                    Knob { label: qsTr("Attack"); value: synthController.modAttack; onMoved: (v) => synthController.modAttack = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Decay"); value: synthController.modDecay; onMoved: (v) => synthController.modDecay = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Intensity"); value: synthController.modInt; onMoved: (v) => synthController.modInt = v; Layout.fillWidth: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox { model: ["Stereo", "Mono", "PingPong", "HiPass", "LowPass", "Tape"]; currentIndex: synthController.delayType; onActivated: (i) => synthController.delayType = i; Layout.fillWidth: true }
                    }
                    RowLayout {
                        CheckBox { text: qsTr("Sync"); checked: synthController.delaySync; onToggled: synthController.delaySync = checked }
                        ComboBox { 
                            visible: synthController.delaySync
                            model: ["1/16", "1/8", "1/4", "1/2", "1/1"]
                            currentIndex: [0.0625, 0.125, 0.25, 0.5, 1.0].indexOf(synthController.delaySyncDivision / 100.0)
                            onActivated: (i) => synthController.delaySyncDivision = [6.25, 12.5, 25, 50, 100][i]
                            Layout.fillWidth: true 
                        }
                        Knob { 
                            visible: !synthController.delaySync
                            label: qsTr("Time"); from: 1; to: 2000; suffix: "ms"; value: synthController.delayTime; onMoved: (v) => synthController.delayTime = v; Layout.fillWidth: true 
                        }
                    }
                    Knob { label: qsTr("Feedback"); value: synthController.delayFeedback; onMoved: (v) => synthController.delayFeedback = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Depth"); value: synthController.delayDepth; onMoved: (v) => synthController.delayDepth = v; Layout.fillWidth: true }
                    Knob { label: qsTr("Mix"); value: synthController.delayMix; onMoved: (v) => synthController.delayMix = v; Layout.fillWidth: true }
                }
            }
        }

        // Test Pads
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Repeater {
                model: [48, 50, 52, 53, 55, 57, 59, 60] // C3 Scale
                Button {
                    text: ["C", "D", "E", "F", "G", "A", "B", "C"][index]
                    Layout.fillWidth: true
                    onPressed: synthController.playNote(modelData, 0.8)
                    onReleased: synthController.stopNote(modelData)
                }
            }
        }
    }
}
