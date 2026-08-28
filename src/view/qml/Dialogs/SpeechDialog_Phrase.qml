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
    spacing: 10

    Label {
        text: qsTr("Phrase")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    TextField {
        id: phraseField
        text: speechController.phrase
        placeholderText: qsTr("Type what it should say")
        selectByMouse: true
        Layout.fillWidth: true
        // Committed on edit rather than per keystroke: recompiling the phrase reallocates the
        // sequencer's phoneme list, which must not happen once per letter while a song is playing.
        onEditingFinished: speechController.phrase = text
        // Enter commits and hands focus back, so the phoneme readout below can be read without
        // having to click elsewhere first. Deliberately does not accept the dialog: typing a phrase
        // is something you do repeatedly while listening, not the last thing you do before Ok.
        Keys.onReturnPressed: focus = false
        Keys.onEnterPressed: focus = false
        Connections {
            target: speechController
            function onPhraseChanged() {
                if (!phraseField.activeFocus) {
                    phraseField.text = speechController.phrase;
                }
            }
        }
    }

    Label {
        text: qsTr("Anything between slashes is read as literal phonemes: hello /w er l d/\nAn apostrophe marks the stressed syllable: A'merica")
        color: "#999"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Phonemes")
        font.bold: true
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    // What the rules made of the phrase. Without it there is no way to tell a bad rule match from a
    // bad formant, and the two need completely different fixes.
    Rectangle {
        color: "#141414"
        border.color: "#333"
        radius: 2
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(60, phonemeLabel.implicitHeight + 16)
        Label {
            id: phonemeLabel
            anchors.fill: parent
            anchors.margins: 8
            text: speechController.phrasePhonemes
            color: "#ddd"
            font.family: "monospace"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    Label {
        text: qsTr("%1 syllable(s)").arg(speechController.syllableCount)
        color: "#999"
        font.pixelSize: 11
    }

    Item {
        Layout.fillHeight: true
    }
}
