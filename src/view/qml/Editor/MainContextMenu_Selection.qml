import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."
import "../Components"

Menu {
    title: qsTr("Selection")
    width: rootItem.width
    Action {
        text: qsTr("Cut")
        shortcut: "Ctrl+X"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionCut()
    }
    Action {
        text: qsTr("Copy")
        shortcut: "Ctrl+C"
        onTriggered: editorService.requestSelectionCopy()
    }
    Action {
        text: qsTr("Paste")
        shortcut: "Ctrl+V"
        enabled: !UiService.isPlaying() && editorService.hasSelectionToPaste
        onTriggered: editorService.requestSelectionPaste()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Transpose <b>+1</b> semitones")
        shortcut: "Alt+Shift+F10"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(1)
    }
    Action {
        text: qsTr("Transpose <b>-1</b> semitones")
        shortcut: "Alt+Shift+F9"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(-1)
    }
    Action {
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(-2)
    }
    Action {
        text: qsTr("Transpose <b>+6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(6)
    }
    Action {
        text: qsTr("Transpose <b>-6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(-6)
    }
    Action {
        text: qsTr("Transpose <b>+12</b> semitones")
        shortcut: "Alt+Shift+F12"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(12)
    }
    Action {
        text: qsTr("Transpose <b>-12</b> semitones")
        shortcut: "Alt+Shift+F11"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSelectionTranspose(-12)
    }
    MenuSeparator {}
    Action {
        text: qsTr("Interpolate velocity")
        enabled: !UiService.isPlaying() && selectionService.isValidSelection
        onTriggered: UiService.requestSelectionVelocityInterpolationDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add MIDI CC automation")
        enabled: !UiService.isPlaying() && selectionService.isValidSelection
        onTriggered: UiService.requestSelectionAddMidiCcAutomationDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Pitch Bend automation")
        enabled: !UiService.isPlaying() && selectionService.isValidSelection
        onTriggered: UiService.requestSelectionAddPitchBendAutomationDialog()
    }
    delegate: MenuItemDelegate {}
}
