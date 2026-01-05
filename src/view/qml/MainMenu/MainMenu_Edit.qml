import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("&Edit")
    Action {
        text: qsTr("Toggle edit mode")
        shortcut: "Esc"
        onTriggered: applicationService.toggleEditMode()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Undo")
        shortcut: "Ctrl+Z"
        enabled: editorService.canUndo
        onTriggered: editorService.undo()
    }
    Action {
        text: qsTr("Redo")
        shortcut: "Ctrl+Y"
        enabled: editorService.canRedo
        onTriggered: editorService.redo()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Reset instruments")
        onTriggered: applicationService.requestInstrumentReset()
    }
    Action {
        text: qsTr("Stop all notes")
        onTriggered: applicationService.requestAllNotesOff()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Settings")
        onTriggered: UiService.requestSettingsDialog()
    }
    delegate: MenuItemDelegate {}
}
