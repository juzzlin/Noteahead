import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    title: qsTr("Keyboard Shortcuts")
    modal: true
    standardButtons: DialogButtonBox.Ok
    visible: false
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
            key: qsTr("Ctrl + C"),
            description: qsTr("Cut the current selection")
        },
        {
            key: qsTr("Ctrl + X"),
            description: qsTr("Copy the current selection")
        },
        {
            key: qsTr("Ctrl + V"),
            description: qsTr("Paste the copied selection")
        },
        // --- Transposition ---
        {
            key: qsTr("Alt + F9"),
            description: qsTr("Transpose column by +1 semitone")
        },
        {
            key: qsTr("Alt + F10"),
            description: qsTr("Transpose column by -1 semitone")
        },
        {
            key: qsTr("Alt + F11"),
            description: qsTr("Transpose column by +12 semitones")
        },
        {
            key: qsTr("Alt + F12"),
            description: qsTr("Transpose column by -12 semitones")
        },
        {
            key: qsTr("Shift + F9"),
            description: qsTr("Transpose track by +1 semitone")
        },
        {
            key: qsTr("Shift + F10"),
            description: qsTr("Transpose track by -1 semitone")
        },
        {
            key: qsTr("Shift + F11"),
            description: qsTr("Transpose track by +12 semitones")
        },
        {
            key: qsTr("Shift + F12"),
            description: qsTr("Transpose track by -12 semitones")
        },
        {
            key: qsTr("Ctrl + F9"),
            description: qsTr("Transpose pattern by +1 semitone")
        },
        {
            key: qsTr("Ctrl + F10"),
            description: qsTr("Transpose pattern by -1 semitone")
        },
        {
            key: qsTr("Ctrl + F11"),
            description: qsTr("Transpose pattern by +12 semitones")
        },
        {
            key: qsTr("Ctrl + F12"),
            description: qsTr("Transpose pattern by -12 semitones")
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
                        color: "orange"
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
