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
        onTriggered: UiService.toggleEditMode()
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
