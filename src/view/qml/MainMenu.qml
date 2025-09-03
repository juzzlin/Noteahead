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
        Action {
            text: qsTr("Recent files")
            onTriggered: UiService.requestRecentFilesDialog()
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
        Action {
            text: qsTr("Save as a template...")
            onTriggered: applicationService.requestSaveProjectAsTemplate()
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
            text: qsTr("Reset instruments")
            onTriggered: applicationService.requestInstrumentReset()
        }
        Action {
            text: qsTr("Stop all notes")
            onTriggered: applicationService.requestAllNotesOff()
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
        title: qsTr("&Tools")
        Action {
            text: qsTr("Delay time calculator")
            onTriggered: UiService.requestDelayCalculatorDialog()
        }
        Action {
            text: qsTr("Note frequencies")
            onTriggered: UiService.requestNoteFrequencyDialog()
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
