import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

MenuBar {
    Menu {
        title: qsTr("&File")
        Action {
            text: qsTr("&New...")
            onTriggered: applicationService.requestNewProject()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("&Open...")
            onTriggered: applicationService.requestOpenProject()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("&Save")
            onTriggered: applicationService.requestSaveProject()
            enabled: editorService.canBeSaved()
        }
        Action {
            text: qsTr("Save &as...")
            onTriggered: applicationService.requestSaveProjectAs()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("&Quit")
            onTriggered: close()
        }
    }
    Menu {
        title: qsTr("&Help")
        Action {
            text: qsTr("&About")
        }
    }
}
