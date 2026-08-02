import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("&Help")
    Action {
        text: qsTr("About")
        onTriggered: UiService.requestAboutDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("User Manual")
        onTriggered: UiService.requestManualDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Shortcuts")
        onTriggered: UiService.requestShortcutsDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("What's New")
        onTriggered: UiService.requestWhatsNewDialog()
    }
    delegate: MenuItemDelegate {}
}
