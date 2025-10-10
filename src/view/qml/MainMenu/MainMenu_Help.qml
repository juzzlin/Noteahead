import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Menu {
    title: qsTr("&Help")
    Action {
        text: qsTr("About")
        onTriggered: UiService.requestAboutDialog()
    }
    delegate: MainMenuItemDelegate {}
}
