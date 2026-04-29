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
    title: qsTr("Noteahead Synth")
    modal: true
    focus: true
    width: 1000
    height: 800

    Universal.accent: themeService.accentColor

    onAboutToShow: () => {
        synthController.initialize();
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

    component OscillatorControl : ColumnLayout {
        id: oscControl
        property string title: "OSC"
        property int waveform
        property real level
        property int octave
        property real tune

        signal waveformEdited(int value)
        signal levelEdited(real value)
        signal octaveEdited(int value)
        signal tuneEdited(real value)

        spacing: 5

        Label {
            text: oscControl.title
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }

        Label { text: qsTr("Waveform"); Layout.alignment: Qt.AlignHCenter }
        ComboBox {
            id: waveformCombo
            model: ["Triangle", "Saw", "Pulse"]
            Layout.fillWidth: true
            currentIndex: oscControl.waveform
            onActivated: (index) => oscControl.waveformEdited(index)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Select the oscillator waveform shape")
        }

        Label { text: qsTr("Octave"); Layout.alignment: Qt.AlignHCenter }
        ComboBox {
            id: octaveCombo
            model: ["16'", "8'", "4'", "2'"]
            Layout.fillWidth: true
            currentIndex: oscControl.octave
            onActivated: (index) => oscControl.octaveEdited(index)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Set the oscillator pitch in octaves")
        }

        Label { text: qsTr("Level") + " (" + levelSlider.value + "%)"; Layout.alignment: Qt.AlignHCenter }
        Slider {
            id: levelSlider
            from: 0
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            value: oscControl.level
            onMoved: () => oscControl.levelEdited(value)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Adjust the volume level of this oscillator")
        }

        Label { text: qsTr("Tune") + " (" + tuneSlider.value + ")"; Layout.alignment: Qt.AlignHCenter }
        Slider {
            id: tuneSlider
            from: -100
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            value: oscControl.tune
            onMoved: () => oscControl.tuneEdited(value)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Fine-tune the oscillator pitch in cents")
        }
    }

    component AdsrControl : ColumnLayout {
        id: adsrControl
        property string title: "ADSR"
        property real attack
        property real decay
        property real sustain
        property real release

        signal attackEdited(real value)
        signal decayEdited(real value)
        signal sustainEdited(real value)
        signal releaseEdited(real value)

        spacing: 5

        Label {
            text: adsrControl.title
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            Label { text: qsTr("A") + " (" + attackSlider.value + ")" }
            Slider {
                id: attackSlider
                from: 0; to: 100; stepSize: 1; Layout.fillWidth: true
                value: adsrControl.attack
                onMoved: () => adsrControl.attackEdited(value)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set the time it takes for the signal to reach maximum level")
            }
            Label { text: qsTr("D") + " (" + decaySlider.value + ")" }
            Slider {
                id: decaySlider
                from: 0; to: 100; stepSize: 1; Layout.fillWidth: true
                value: adsrControl.decay
                onMoved: () => adsrControl.decayEdited(value)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set the time it takes for the signal to fall to the sustain level")
            }
            Label { text: qsTr("S") + " (" + sustainSlider.value + "%)" }
            Slider {
                id: sustainSlider
                from: 0; to: 100; stepSize: 1; Layout.fillWidth: true
                value: adsrControl.sustain
                onMoved: () => adsrControl.sustainEdited(value)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set the level held while the note is sustained")
            }
            Label { text: qsTr("R") + " (" + releaseSlider.value + ")" }
            Slider {
                id: releaseSlider
                from: 0; to: 100; stepSize: 1; Layout.fillWidth: true
                value: adsrControl.release
                onMoved: () => adsrControl.releaseEdited(value)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set the time it takes for the signal to fall to zero after the note is released")
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.preferredWidth: resetBtn.width }
            Text {
                text: qsTr("Noteahead Synth")
                color: "white"
                font.bold: true
                font.pixelSize: 20
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                id: resetBtn
                text: qsTr("Reset")
                implicitWidth: Constants.defaultButtonWidth
                onClicked: synthController.reset()
            }
        }

        ScrollView {
            id: settingsScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: settingsScrollView.availableWidth
                spacing: 20

                // Oscillators Row
                RowLayout {
                    spacing: 20
                    Layout.fillWidth: true

                    OscillatorControl {
                        title: "OSC 1"
                        Layout.fillWidth: true
                        waveform: synthController.osc1Waveform
                        onWaveformEdited: (value) => synthController.osc1Waveform = value
                        level: synthController.osc1Level
                        onLevelEdited: (value) => synthController.osc1Level = value
                        octave: synthController.osc1Octave + 1
                        onOctaveEdited: (value) => synthController.osc1Octave = value - 1
                        tune: synthController.osc1Tune
                        onTuneEdited: (value) => synthController.osc1Tune = value
                    }

                    OscillatorControl {
                        title: "OSC 2"
                        Layout.fillWidth: true
                        waveform: synthController.osc2Waveform
                        onWaveformEdited: (value) => synthController.osc2Waveform = value
                        level: synthController.osc2Level
                        onLevelEdited: (value) => synthController.osc2Level = value
                        octave: synthController.osc2Octave + 1
                        onOctaveEdited: (value) => synthController.osc2Octave = value - 1
                        tune: synthController.osc2Tune
                        onTuneEdited: (value) => synthController.osc2Tune = value
                    }

                    OscillatorControl {
                        title: "OSC 3"
                        Layout.fillWidth: true
                        waveform: synthController.osc3Waveform
                        onWaveformEdited: (value) => synthController.osc3Waveform = value
                        level: synthController.osc3Level
                        onLevelEdited: (value) => synthController.osc3Level = value
                        octave: synthController.osc3Octave + 1
                        onOctaveEdited: (value) => synthController.osc3Octave = value - 1
                        tune: synthController.osc3Tune
                        onTuneEdited: (value) => synthController.osc3Tune = value
                    }

                    OscillatorControl {
                        title: "OSC 4"
                        Layout.fillWidth: true
                        waveform: synthController.osc4Waveform
                        onWaveformEdited: (value) => synthController.osc4Waveform = value
                        level: synthController.osc4Level
                        onLevelEdited: (value) => synthController.osc4Level = value
                        octave: synthController.osc4Octave + 1
                        onOctaveEdited: (value) => synthController.osc4Octave = value - 1
                        tune: synthController.osc4Tune
                        onTuneEdited: (value) => synthController.osc4Tune = value
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.1)
                }

                // VCF & Envelopes Row
                RowLayout {
                    spacing: 40
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("VCF")
                            font.bold: true
                            font.pixelSize: 16
                        }
                        GridLayout {
                            columns: 2
                            Layout.fillWidth: true
                            Label { text: qsTr("Cutoff") + " (" + filterCutoffSlider.value + "%)" }
                            Slider {
                                id: filterCutoffSlider
                                from: 0
                                to: 100
                                stepSize: 1
                                value: synthController.filterCutoff
                                onMoved: () => synthController.filterCutoff = filterCutoffSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Frequency above which the filter starts to attenuate the signal")
                            }
                            Label { text: qsTr("Resonance") + " (" + filterResonanceSlider.value + "%)" }
                            Slider {
                                id: filterResonanceSlider
                                from: 0
                                to: 100
                                stepSize: 1
                                value: synthController.filterResonance
                                onMoved: () => synthController.filterResonance = filterResonanceSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Emphasize the frequencies around the cutoff point")
                            }
                            Label { text: qsTr("Env Amount") + " (" + filterEnvAmountSlider.value + "%)" }
                            Slider {
                                id: filterEnvAmountSlider
                                from: -100
                                to: 100
                                stepSize: 1
                                value: synthController.filterEnvAmount
                                onMoved: () => synthController.filterEnvAmount = filterEnvAmountSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Amount of envelope modulation applied to the filter cutoff")
                            }
                            Label { text: qsTr("Key Track") + " (" + filterKeyTrackSlider.value + "%)" }
                            Slider {
                                id: filterKeyTrackSlider
                                from: 0
                                to: 100
                                stepSize: 1
                                value: synthController.filterKeyTrack
                                onMoved: () => synthController.filterKeyTrack = filterKeyTrackSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("How much the filter cutoff follows the keyboard pitch")
                            }
                            Label { text: qsTr("Mod Amt") + " (" + filterModAmountSlider.value + "%)" }
                            Slider {
                                id: filterModAmountSlider
                                from: 0
                                to: 100
                                stepSize: 1
                                value: synthController.filterModAmount
                                onMoved: () => synthController.filterModAmount = filterModAmountSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Amount of filter modulation from MG 2")
                            }
                            Label { text: qsTr("Pulse Width") + " (" + pulseWidthSlider.value + "%)" }
                            Slider {
                                id: pulseWidthSlider
                                from: 1
                                to: 99
                                stepSize: 1
                                value: synthController.pulseWidth
                                onMoved: () => synthController.pulseWidth = pulseWidthSlider.value
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Adjust the pulse width of the square waveform")
                            }
                            }
                            }
                    AdsrControl {
                        title: "VCF EG"
                        Layout.fillWidth: true
                        attack: synthController.vcfAttack
                        onAttackEdited: (value) => synthController.vcfAttack = value
                        decay: synthController.vcfDecay
                        onDecayEdited: (value) => synthController.vcfDecay = value
                        sustain: synthController.vcfSustain
                        onSustainEdited: (value) => synthController.vcfSustain = value
                        release: synthController.vcfRelease
                        onReleaseEdited: (value) => synthController.vcfRelease = value
                    }

                    AdsrControl {
                        title: "VCA EG"
                        Layout.fillWidth: true
                        attack: synthController.vcaAttack
                        onAttackEdited: (value) => synthController.vcaAttack = value
                        decay: synthController.vcaDecay
                        onDecayEdited: (value) => synthController.vcaDecay = value
                        sustain: synthController.vcaSustain
                        onSustainEdited: (value) => synthController.vcaSustain = value
                        release: synthController.vcaRelease
                        onReleaseEdited: (value) => synthController.vcaRelease = value
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.1)
                }

                // MG/LFO Row
                RowLayout {
                    spacing: 40
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Freq Mod")
                            font.bold: true
                            font.pixelSize: 16
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Amt") + " (" + freqModAmountSlider.value + "%)" }
                            Slider {
                                id: freqModAmountSlider
                                Layout.fillWidth: true
                                from: 0
                                to: 100
                                stepSize: 1
                                value: synthController.freqModAmount
                                onMoved: () => synthController.freqModAmount = freqModAmountSlider.value
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Amount of frequency modulation")
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Source") }
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["MG 1", "VCF EG"]
                                currentIndex: synthController.freqModSource
                                onActivated: (index) => synthController.freqModSource = index
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Source for frequency modulation")
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("MG 1 (Pitch)")
                            font.bold: true
                            font.pixelSize: 16
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Freq") + " (" + mg1FreqSlider.value + ")" }
                            Slider {
                                id: mg1FreqSlider
                                Layout.fillWidth: true
                                from: 1
                                to: 200
                                stepSize: 1
                                value: synthController.mg1Frequency
                                onMoved: () => synthController.mg1Frequency = mg1FreqSlider.value
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Pitch modulation generator (LFO) frequency")
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("MG 2 (Filter)")
                            font.bold: true
                            font.pixelSize: 16
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Freq") + " (" + mg2FreqSlider.value + ")" }
                            Slider {
                                id: mg2FreqSlider
                                Layout.fillWidth: true
                                from: 1
                                to: 200
                                stepSize: 1
                                value: synthController.mg2Frequency
                                onMoved: () => synthController.mg2Frequency = mg2FreqSlider.value
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Filter modulation generator (LFO) frequency")
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Detune") + " (" + globalDetuneSlider.value + ")"
                            font.bold: true
                        }
                        Slider {
                            id: globalDetuneSlider
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            stepSize: 1
                            value: synthController.detune
                            onMoved: () => synthController.detune = globalDetuneSlider.value
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Global detune for all oscillators")
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Volume") + " (" + globalVolumeSlider.value + "%)"
                            font.bold: true
                        }
                        Slider {
                            id: globalVolumeSlider
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            stepSize: 1
                            value: synthController.volume
                            onMoved: () => synthController.volume = globalVolumeSlider.value
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Global master volume for the synth")
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Mode")
                            font.bold: true
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Unison", "Unison Share", "Poly"]
                            currentIndex: synthController.keyAssignMode
                            onActivated: (index) => synthController.keyAssignMode = index
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Voice assignment mode (Unison, Poly, etc.)")
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Qt.rgba(1, 1, 1, 0.1)
        }

        // Test Pads Row
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10
            Label {
                text: qsTr("Test Pads (C-keys)")
                font.bold: true
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Repeater {
                    model: [0, 12, 24, 36, 48, 60, 72, 84, 96, 108]
                    delegate: Rectangle {
                        id: pad
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        radius: 8
                        color: mouseArea.pressed ? themeService.accentColor : Qt.rgba(1, 1, 1, 0.05)
                        border.color: mouseArea.pressed ? "white" : Qt.rgba(1, 1, 1, 0.2)
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "C" + Math.floor(modelData / 12)
                            font.bold: true
                            color: mouseArea.pressed ? "white" : "#ccc"
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onPressed: (mouse) => synthController.playNote(modelData, 0.8)
                            onReleased: (mouse) => synthController.stopNote(modelData)
                            onCanceled: () => synthController.stopNote(modelData)
                        }
                    }
                }
            }
        }
    }
}
