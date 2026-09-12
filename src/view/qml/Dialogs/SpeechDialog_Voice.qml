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

    Label {
        text: qsTr("Voice")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Type")
        }
        ComboBox {
            // Read through the active language so retranslate() reaches a model built in JS.
            model: {
                languageService.activeLanguage;
                return [qsTr("Male"), qsTr("Female"), qsTr("Child"), qsTr("Deep"), qsTr("Breathy")];
            }
            currentIndex: speechController.voiceType
            onActivated: i => speechController.voiceType = i
            Layout.fillWidth: true
        }
    }

    Knob {
        label: qsTr("Formant Shift")
        value: speechController.formantShift
        onMoved: v => speechController.formantShift = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Vocal tract length. Up reads as a smaller speaker.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Glide")
        value: speechController.glide
        onMoved: v => speechController.glide = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long a formant takes to reach its target. Short is robotic, long is slurred.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Consonants")
        value: speechController.consonantLevel
        onMoved: v => speechController.consonantLevel = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Level of everything that is not a vowel.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sibilance")
        value: speechController.sibilance
        onMoved: v => speechController.sibilance = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How loud and how bright the s-sounds are. Half way is what the phoneme table asks for.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Breathiness")
        value: speechController.breathiness
        onMoved: v => speechController.breathiness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Openness")
        enabled: speechController.voiceEngine === 1
        value: speechController.openQuotient
        onMoved: v => speechController.openQuotient = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long the vocal folds stay open. Down is pressed and buzzy, up is soft and breathy. Half way is what the voice type asks for.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Jitter")
        enabled: speechController.voiceEngine === 1
        value: speechController.voicePerturbation
        onMoved: v => speechController.voicePerturbation = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How unsteady the voice is from one cycle to the next. At zero it is perfectly periodic, which is what a machine sounds like.")
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Glottal Source")
        checked: speechController.voiceEngine === 1
        onToggled: speechController.voiceEngine = checked ? 1 : 0
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Models the vocal folds instead of the sawtooth this device used before. Off in songs saved with that older voice, so they sound as they did; turning it on changes how they sound.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Intonation")
        value: speechController.intonation
        onMoved: v => speechController.intonation = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the pitch falls across a phrase. At zero every phrase is a monotone.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Vibrato Rate")
        value: speechController.vibratoRate
        onMoved: v => speechController.vibratoRate = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Vibrato Depth")
        value: speechController.vibratoDepth
        onMoved: v => speechController.vibratoDepth = v
        Layout.fillWidth: true
    }

    Item {
        Layout.fillHeight: true
    }
}
