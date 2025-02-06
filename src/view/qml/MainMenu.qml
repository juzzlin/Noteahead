import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

MenuBar {
    Menu {
        title: qsTr("&File")
        Action {
            text: qsTr("New...")
            shortcut: "Ctrl+N"
            onTriggered: applicationService.requestNewProject()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Open...")
            shortcut: "Ctrl+O"
            onTriggered: applicationService.requestOpenProject()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Save")
            shortcut: "Ctrl+S"
            onTriggered: applicationService.requestSaveProject()
            enabled: editorService.canBeSaved
        }
        Action {
            text: qsTr("Save as...")
            shortcut: "Ctrl+A"
            onTriggered: applicationService.requestSaveProjectAs()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Quit")
            shortcut: "Ctrl+Q"
            onTriggered: UiService.requestQuit()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    Menu {
        title: qsTr("&Edit")
        Action {
            text: qsTr("Toggle edit mode")
            shortcut: "Esc"
            onTriggered: UiService.toggleEditMode()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Settings")
            onTriggered: UiService.requestSettingsDialog()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    Menu {
        title: qsTr("&Help")
        Action {
            text: qsTr("About")
            onTriggered: UiService.requestAboutDialog()
        }
        delegate: MainMenuItemDelegate {
        }
    }
}
