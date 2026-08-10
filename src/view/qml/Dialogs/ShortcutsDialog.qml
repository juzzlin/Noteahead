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
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

AnimatedDialog {
    id: rootItem
    title: qsTr("Keyboard Shortcuts")
    modal: true
    visible: false

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
    readonly property var shortcuts: [
        // --- Basic Controls ---
        {
            key: qsTr("ESC"),
            description: qsTr("Toggle edit mode")
        },
        {
            key: qsTr("SPACE"),
            description: qsTr("Toggle play mode")
        },
        {
            key: qsTr("F5"),
            description: qsTr("Start playing from the current position")
        },
        {
            key: qsTr("F6"),
            description: qsTr("Stop playing")
        },
        {
            key: qsTr("F8"),
            description: qsTr("Rewind to the start of the song and reset track settings")
        },
        {
            key: qsTr("INSERT"),
            description: qsTr("Insert empty line and move subsequent lines down")
        },
        {
            key: qsTr("BACKSPACE"),
            description: qsTr("Delete current line and pull subsequent lines up")
        },
        {
            key: qsTr("A"),
            description: qsTr("Insert note off event")
        },
        {
            key: "",
            description: ""
        },
        {
            key: qsTr("F3"),
            description: qsTr("Decrease current octave")
        },
        {
            key: qsTr("F4"),
            description: qsTr("Increase current octave")
        },
        {
            key: qsTr("Z–M"),
            description: qsTr("Play/insert notes of the lower octave")
        },
        {
            key: qsTr("Q–U"),
            description: qsTr("Play/insert notes of the higher octave")
        },
        // --- Cut / Copy / Paste ---
        {
            key: qsTr("Alt + F3"),
            description: qsTr("Cut the current column")
        },
        {
            key: qsTr("Alt + F4"),
            description: qsTr("Copy the current column")
        },
        {
            key: qsTr("Alt + F5"),
            description: qsTr("Paste the copied column")
        },
        {
            key: qsTr("Shift + F3"),
            description: qsTr("Cut the current track")
        },
        {
            key: qsTr("Shift + F4"),
            description: qsTr("Copy the current track")
        },
        {
            key: qsTr("Shift + F5"),
            description: qsTr("Paste the copied track")
        },
        {
            key: qsTr("Ctrl + F3"),
            description: qsTr("Cut the current pattern")
        },
        {
            key: qsTr("Ctrl + F4"),
            description: qsTr("Copy the current pattern")
        },
        {
            key: qsTr("Ctrl + F5"),
            description: qsTr("Paste the copied pattern")
        },
        {
            key: qsTr("Ctrl + X"),
            description: qsTr("Cut the current selection")
        },
        {
            key: qsTr("Ctrl + C"),
            description: qsTr("Copy the current selection")
        },
        {
            key: qsTr("Ctrl + V"),
            description: qsTr("Paste the copied selection")
        },
        // --- Transposition ---
        {
            key: qsTr("Alt + F9"),
            description: qsTr("Transpose column by -1 semitone")
        },
        {
            key: qsTr("Alt + F10"),
            description: qsTr("Transpose column by +1 semitone")
        },
        {
            key: qsTr("Alt + F11"),
            description: qsTr("Transpose column by -12 semitones")
        },
        {
            key: qsTr("Alt + F12"),
            description: qsTr("Transpose column by +12 semitones")
        },
        {
            key: qsTr("Shift + F9"),
            description: qsTr("Transpose track by -1 semitone")
        },
        {
            key: qsTr("Shift + F10"),
            description: qsTr("Transpose track by +1 semitone")
        },
        {
            key: qsTr("Shift + F11"),
            description: qsTr("Transpose track by -12 semitones")
        },
        {
            key: qsTr("Shift + F12"),
            description: qsTr("Transpose track by +12 semitones")
        },
        {
            key: qsTr("Ctrl + F9"),
            description: qsTr("Transpose pattern by -1 semitone")
        },
        {
            key: qsTr("Ctrl + F10"),
            description: qsTr("Transpose pattern by +1 semitone")
        },
        {
            key: qsTr("Ctrl + F11"),
            description: qsTr("Transpose pattern by -12 semitones")
        },
        {
            key: qsTr("Ctrl + F12"),
            description: qsTr("Transpose pattern by +12 semitones")
        }
    ]
    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        GridLayout {
            id: grid
            columns: 2
            columnSpacing: 40
            rowSpacing: 20
            anchors.margins: 10
            width: rootItem.width
            Repeater {
                model: rootItem.shortcuts
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: modelData.key
                        font.family: "monospace"
                        color: themeService.accentColor
                        wrapMode: Text.WrapAnywhere
                    }
                    Label {
                        text: modelData.description
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
    Component.onCompleted: visible = false
}
