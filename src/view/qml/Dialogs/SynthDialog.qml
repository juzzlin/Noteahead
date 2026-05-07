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
    title: applicationService.synthDeviceName
    modal: true
    focus: true
    width: 1000
    height: 850

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    property bool isSaving: false

    onAboutToShow: () => {
        isSaving = false;
        synthController.requestSettings();
    }

    StringInputDialog {
        id: presetNameDialog
        onAccepted: {
            synthController.saveUserPreset(text)
            isSaving = false
        }
        onRejected: {
            isSaving = false
        }
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
            isSaving = false;
            synthController.accept();
        }
        onRejected: () => {
            isSaving = false;
            synthController.reject();
        }
    }

    component SectionTitle: Label {
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
            SectionTitle {
                text: isSaving ? qsTr("Select slot to save preset...") : qsTr("A general purpose 6-voice synthesizer")
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Bank:")
            }
            ComboBox {
                id: bankCombo
                model: [qsTr("Factory"), qsTr("User")]
                currentIndex: synthController.currentBank
                onActivated: index => {
                    synthController.currentBank = index
                }
            }

            Label {
                text: qsTr("Preset:")
            }
            ComboBox {
                id: presetCombo
                implicitWidth: 300
                model: synthController.currentBank === 0 ? synthController.presetNames : synthController.userPresetNames
                currentIndex: synthController.currentPresetIndex
                onActivated: index => {
                    if (isSaving) {
                        synthController.currentPresetIndex = index
                        presetNameDialog.setTitle(qsTr("Save User Preset"))
                        presetNameDialog.text = synthController.userPresetNames[index].split(": ")[1]
                        presetNameDialog.open()
                    } else {
                        synthController.currentPresetIndex = index
                        synthController.loadPreset(index)
                    }
                }

                delegate: ItemDelegate {
                    width: 235
                    highlighted: presetCombo.highlightedIndex === index
                    contentItem: Label {
                        text: modelData
                        color: presetCombo.currentIndex === index ? themeService.accentColor : "white"
                        font.bold: presetCombo.currentIndex === index
                        elide: Text.ElideRight
                        verticalAlignment: Image.AlignVCenter
                    }
                    background: Rectangle {
                        color: highlighted ? "#333" : "transparent"
                    }
                    onClicked: {
                        presetCombo.currentIndex = index;
                        presetCombo.activated(index);
                        presetCombo.popup.close();
                    }
                }

                popup: Popup {
                    y: presetCombo.height
                    x: (root.width - width) / 2 - parent.x - 15
                    width: 980
                    implicitHeight: 500
                    padding: 5

                    contentItem: GridView {
                        clip: true
                        model: presetCombo.delegateModel
                        cellWidth: 240
                        cellHeight: 40
                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AlwaysOn
                        }
                    }

                    background: Rectangle {
                        color: "#252525"
                        border.color: themeService.accentColor
                        radius: 4
                    }
                }
            }

            Button {
                text: qsTr("Save")
                highlighted: isSaving
                onClicked: {
                    isSaving = true
                    synthController.currentBank = 1
                    presetCombo.popup.open()
                }
            }

            Button {
                text: qsTr("Reset")
                onClicked: {
                    isSaving = false
                    synthController.reset()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridLayout {
                columns: 5
                columnSpacing: 20
                rowSpacing: 10
                width: parent.width - 20

                // Row 1: Titles
                SectionTitle {
                    text: "Voice / Global"
                }
                SectionTitle {
                    text: "VCO 1"
                }
                SectionTitle {
                    text: "VCO 2"
                }
                SectionTitle {
                    text: "Multi Engine"
                }
                SectionTitle {
                    text: "Filter"
                }

                // Row 2: Controls
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Poly", "Unison"]
                            currentIndex: synthController.voiceMode
                            onActivated: i => synthController.voiceMode = i
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Voice Depth")
                        value: synthController.voiceDepth
                        onMoved: v => synthController.voiceDepth = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Portamento")
                        value: synthController.portamento
                        onMoved: v => synthController.portamento = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Pan Spread")
                        value: synthController.panSpread
                        onMoved: v => synthController.panSpread = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Master Volume")
                        value: synthController.volume
                        onMoved: v => synthController.volume = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Gain")
                        suffix: "dB"
                        value: synthController.gain
                        onMoved: v => synthController.gain = v
                        Layout.fillWidth: true
                    }
                    PanKnob {
                        label: qsTr("Master Pan")
                        value: synthController.pan
                        onMoved: v => synthController.pan = v
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Triangle", "Saw", "Pulse"]
                            currentIndex: synthController.vco1Waveform
                            onActivated: i => synthController.vco1Waveform = i
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            model: ["16'", "8'", "4'", "2'"]
                            currentIndex: synthController.vco1Octave + 1
                            onActivated: i => synthController.vco1Octave = i - 1
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Pitch")
                        from: -Constants.uiInternalScaling
                        to: Constants.uiInternalScaling
                        suffix: "c"
                        value: synthController.pitchToUiValue(synthController.vco1Pitch)
                        customValue: synthController.vco1Pitch
                        onMoved: v => synthController.vco1Pitch = synthController.uiValueToPitch(v)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Shape")
                        value: synthController.vco1Shape
                        onMoved: v => synthController.vco1Shape = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Level")
                        value: synthController.mixVco1
                        onMoved: v => synthController.mixVco1 = v
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        text: qsTr("Phase Sync")
                        checked: synthController.vco1Sync
                        onToggled: synthController.vco1Sync = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Triangle", "Saw", "Pulse"]
                            currentIndex: synthController.vco2Waveform
                            onActivated: i => synthController.vco2Waveform = i
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            model: ["16'", "8'", "4'", "2'"]
                            currentIndex: synthController.vco2Octave + 1
                            onActivated: i => synthController.vco2Octave = i - 1
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Pitch")
                        from: -Constants.uiInternalScaling
                        to: Constants.uiInternalScaling
                        suffix: "c"
                        value: synthController.pitchToUiValue(synthController.vco2Pitch)
                        customValue: synthController.vco2Pitch
                        onMoved: v => synthController.vco2Pitch = synthController.uiValueToPitch(v)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Shape")
                        value: synthController.vco2Shape
                        onMoved: v => synthController.vco2Shape = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Level")
                        value: synthController.mixVco2
                        onMoved: v => synthController.mixVco2 = v
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        text: qsTr("Hard Sync to VCO1")
                        checked: synthController.vco2Sync
                        onToggled: synthController.vco2Sync = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["High", "Low", "Peak", "Decim"]
                            currentIndex: synthController.multiType
                            onActivated: i => synthController.multiType = i
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Shape")
                        value: synthController.multiShape
                        onMoved: v => synthController.multiShape = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Key Track")
                        value: synthController.multiKeyTrack
                        onMoved: v => synthController.multiKeyTrack = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Level")
                        value: synthController.multiLevel
                        onMoved: v => synthController.multiLevel = v
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    FilterKnob {
                        label: qsTr("LPF Cutoff")
                        controller: synthController
                        value: synthController.lpfCutoff
                        onMoved: v => synthController.lpfCutoff = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("LPF Resonance")
                        value: synthController.lpfResonance
                        onMoved: v => synthController.lpfResonance = v
                        Layout.fillWidth: true
                    }
                    FilterKnob {
                        label: qsTr("HPF Cutoff")
                        controller: synthController
                        value: synthController.hpfCutoff
                        isHpf: true
                        onMoved: v => synthController.hpfCutoff = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Key Track")
                        value: synthController.filterKeyTrack
                        onMoved: v => synthController.filterKeyTrack = v
                        Layout.fillWidth: true
                    }
                }

                // Spacing Row
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    Layout.columnSpan: 5
                }

                // Row 3: Titles
                SectionTitle {
                    text: "LFO"
                }
                SectionTitle {
                    text: "Amp EG (ADSR)"
                }
                SectionTitle {
                    text: "Mod EG (AD)"
                }
                SectionTitle {
                    text: "Delay Effect"
                }
                Item {
                    Layout.fillWidth: true
                }

                // Row 4: Controls
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Saw", "Triangle", "Square"]
                            currentIndex: synthController.lfoWaveform
                            onActivated: i => synthController.lfoWaveform = i
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            model: ["Normal", "BPM", "1-Shot"]
                            currentIndex: synthController.lfoMode
                            onActivated: i => synthController.lfoMode = i
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        ComboBox {
                            model: ["Pitch", "Shape", "Cutoff"]
                            currentIndex: synthController.lfoTarget
                            onActivated: i => synthController.lfoTarget = i
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Rate")
                        value: synthController.lfoRate
                        onMoved: v => synthController.lfoRate = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Intensity")
                        value: synthController.lfoInt
                        onMoved: v => synthController.lfoInt = v
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Knob {
                        label: qsTr("Attack")
                        value: synthController.ampAttack
                        onMoved: v => synthController.ampAttack = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Decay")
                        value: synthController.ampDecay
                        onMoved: v => synthController.ampDecay = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Sustain")
                        value: synthController.ampSustain
                        onMoved: v => synthController.ampSustain = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Release")
                        value: synthController.ampRelease
                        onMoved: v => synthController.ampRelease = v
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Pitch 1", "Pitch 2", "Cutoff"]
                            currentIndex: synthController.modTarget
                            onActivated: i => synthController.modTarget = i
                            Layout.fillWidth: true
                        }
                    }
                    Knob {
                        label: qsTr("Attack")
                        value: synthController.modAttack
                        onMoved: v => synthController.modAttack = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Decay")
                        value: synthController.modDecay
                        onMoved: v => synthController.modDecay = v
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Intensity")
                        value: synthController.modInt
                        onMoved: v => synthController.modInt = v
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        ComboBox {
                            model: ["Stereo", "Mono", "PingPong", "HiPass", "LowPass", "Tape"]
                            currentIndex: synthController.delayType
                            onActivated: i => synthController.delayType = i
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        CheckBox {
                            text: qsTr("Sync")
                            checked: synthController.delaySync
                            onToggled: synthController.delaySync = checked
                        }
                        StackLayout {
                            currentIndex: synthController.delaySync ? 0 : 1
                            Layout.fillWidth: true
                            Layout.preferredHeight: noteDurationCombo.implicitHeight
                            ComboBox {
                                id: noteDurationCombo
                                model: ["1/1", "3/4", "1/2", "3/8", "1/3", "1/4", "3/16", "1/6", "1/8", "3/32", "1/12", "1/16", "3/64", "1/24", "1/32", "1/64"]
                                readonly property var divisions: [1.0, 0.75, 0.5, 0.375, 1/3, 0.25, 0.1875, 1/6, 0.125, 0.09375, 1/12, 0.0625, 0.046875, 1/24, 0.03125, 0.015625]
                                currentIndex: {
                                    const val = synthController.delaySyncDivision / Constants.uiInternalScaling;
                                    let bestIdx = 0;
                                    let minDiff = 10.0; // Larger than max possible difference (1.0)
                                    for (let i = 0; i < divisions.length; ++i) {
                                        let diff = Math.abs(divisions[i] - val);
                                        if (diff < minDiff) {
                                            minDiff = diff;
                                            bestIdx = i;
                                        }
                                    }
                                    return bestIdx;
                                }
                                onActivated: i => {
                                    synthController.delaySyncDivision = divisions[i] * Constants.uiInternalScaling;
                                }
                                Layout.fillWidth: true
                            }
                            Knob {
                                label: qsTr("Time")
                                from: 1
                                to: 10000
                                suffix: "ms"
                                value: synthController.delayTime
                                onMoved: v => synthController.delayTime = v
                                Layout.fillWidth: true
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Knob {
                            label: qsTr("Feedback")
                            value: synthController.delayFeedback
                            onMoved: v => synthController.delayFeedback = v
                            Layout.fillWidth: true
                        }
                        FilterKnob {
                            label: qsTr("LPF")
                            controller: synthController
                            value: synthController.delayFeedbackLpf
                            onMoved: v => synthController.delayFeedbackLpf = v
                            Layout.fillWidth: true
                        }
                        FilterKnob {
                            label: qsTr("HPF")
                            controller: synthController
                            value: synthController.delayFeedbackHpf
                            isHpf: true
                            onMoved: v => synthController.delayFeedbackHpf = v
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Knob {
                            label: qsTr("Depth")
                            value: synthController.delayDepth
                            onMoved: v => synthController.delayDepth = v
                            Layout.fillWidth: true
                        }
                        Knob {
                            label: qsTr("Mix")
                            value: synthController.delayMix
                            onMoved: v => synthController.delayMix = v
                            Layout.fillWidth: true
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 20
            onNoteOnRequested: note => synthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => synthController.stopNote(note)
        }
    }
}
