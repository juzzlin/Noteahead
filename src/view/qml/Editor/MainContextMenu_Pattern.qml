import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."
import "../Components"

Menu {
    title: "Pattern"
    width: rootItem.width
    Action {
        text: qsTr("Cut")
        shortcut: "Ctrl+F3"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternCut()
    }
    Action {
        text: qsTr("Copy")
        shortcut: "Ctrl+F4"
        onTriggered: editorService.requestPatternCopy()
    }
    Action {
        text: qsTr("Paste")
        shortcut: "Ctrl+F5"
        enabled: !UiService.isPlaying() && editorService.hasPatternToPaste
        onTriggered: editorService.requestPatternPaste()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Transpose <b>+1</b> semitones")
        shortcut: "Ctrl+F10"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(1)
    }
    Action {
        text: qsTr("Transpose <b>-1</b> semitones")
        shortcut: "Ctrl+F9"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(-1)
    }
    Action {
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(-2)
    }
    Action {
        text: qsTr("Transpose <b>+6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(6)
    }
    Action {
        text: qsTr("Transpose <b>-6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(-6)
    }
    Action {
        text: qsTr("Transpose <b>+12</b> semitones")
        shortcut: "Ctrl+F12"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(12)
    }
    Action {
        text: qsTr("Transpose <b>-12</b> semitones")
        shortcut: "Ctrl+F11"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternTranspose(-12)
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit MIDI CC automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByPattern()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit Pitch Bend automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByPattern()
    }
    delegate: MenuItemDelegate {}
}
