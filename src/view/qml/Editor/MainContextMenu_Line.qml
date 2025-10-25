import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("Line")
    width: rootItem.width
    Action {
        text: qsTr("Insert an event")
        shortcut: "Alt+E"
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEventSelectionDialog()
    }
    Action {
        text: qsTr("Remove current event")
        shortcut: "Alt+Shift+E"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestEventRemoval()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Set delay")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineDelayDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add MIDI CC automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineAddMidiCcAutomationDialog()
    }
    Action {
        text: qsTr("Edit MIDI CC automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByLine()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Pitch Bend automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineAddPitchBendAutomationDialog()
    }
    Action {
        text: qsTr("Edit Pitch Bend automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByLine()
    }
    delegate: MenuItemDelegate {}
}
