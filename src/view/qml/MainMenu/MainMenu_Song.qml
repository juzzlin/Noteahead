import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("&Song")
    Action {
        text: qsTr("Settings...")
        onTriggered: UiService.requestSongSettingsDialog()
    }
    delegate: MenuItemDelegate {}
}
