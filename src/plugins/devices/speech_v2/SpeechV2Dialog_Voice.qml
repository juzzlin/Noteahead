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
    spacing: 6

    SectionHeading {
        text: qsTr("Voice")
        Layout.topMargin: 10
    }

    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Type")
        }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [
                    qsTr("Male"), qsTr("Female"), qsTr("Child"), qsTr("Deep"), qsTr("Breathy"),
                    qsTr("Giant"), qsTr("Elder"), qsTr("Alien"), qsTr("Monster"),
                    qsTr("Robot"), qsTr("Cyborg"), qsTr("Whisper"), qsTr("Demon"), qsTr("8-Bit"), qsTr("Mutant")
                ];
            }
            currentIndex: speechV2Controller.voiceType
            onActivated: i => speechV2Controller.voiceType = i
            Layout.fillWidth: true
        }
    }

    Knob {
        label: qsTr("Formant Shift")
        value: speechV2Controller.formantShift
        onMoved: v => speechV2Controller.formantShift = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Vocal tract length. Up reads as a smaller speaker.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Glide")
        value: speechV2Controller.glide
        onMoved: v => speechV2Controller.glide = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long a formant takes to reach its target. Short is robotic, long is slurred.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Consonants")
        value: speechV2Controller.consonantLevel
        onMoved: v => speechV2Controller.consonantLevel = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Level of everything that is not a vowel.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sibilance")
        value: speechV2Controller.sibilance
        onMoved: v => speechV2Controller.sibilance = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How loud and how bright the s-sounds are. Half way is what the phoneme table asks for.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Breathiness")
        value: speechV2Controller.breathiness
        onMoved: v => speechV2Controller.breathiness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Openness")
        value: speechV2Controller.openQuotient
        onMoved: v => speechV2Controller.openQuotient = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long the vocal folds stay open. Down is pressed and buzzy, up is soft and breathy. Half way is what the voice type asks for.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Jitter")
        value: speechV2Controller.voicePerturbation
        onMoved: v => speechV2Controller.voicePerturbation = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How unsteady the voice is from one cycle to the next. At zero it is perfectly periodic, which is what a machine sounds like.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Intonation")
        value: speechV2Controller.intonation
        onMoved: v => speechV2Controller.intonation = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the pitch falls across a phrase. At zero every phrase is a monotone.")
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        LfoLed {
            controller: speechV2Controller
            lfoIndex: 0
        }
        Knob {
            label: qsTr("Vibrato Rate")
            value: speechV2Controller.vibratoRate
            onMoved: v => speechV2Controller.vibratoRate = v
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Vibrato Depth")
        value: speechV2Controller.vibratoDepth
        onMoved: v => speechV2Controller.vibratoDepth = v
        Layout.fillWidth: true
    }

    Item {
        Layout.fillHeight: true
    }
}
