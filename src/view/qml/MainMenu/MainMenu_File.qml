import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("&File")
    Action {
        text: qsTr("New...")
        shortcut: "Ctrl+N"
        onTriggered: applicationService.requestNewProject()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Open...")
        shortcut: "Ctrl+O"
        onTriggered: applicationService.requestOpenProject()
    }
    Action {
        text: qsTr("Recent files")
        onTriggered: UiService.requestRecentFilesDialog()
    }
    MenuSeparator {}
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
    MenuSeparator {}
    Action {
        text: qsTr("Quit")
        shortcut: "Ctrl+Q"
        onTriggered: UiService.requestQuit()
    }
    delegate: MenuItemDelegate {}
}
