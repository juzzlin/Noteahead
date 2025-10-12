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
    Action {
        text: qsTr("Shortcuts")
        onTriggered: UiService.requestShortcutsDialog()
    }
    delegate: MenuItemDelegate {}
}
