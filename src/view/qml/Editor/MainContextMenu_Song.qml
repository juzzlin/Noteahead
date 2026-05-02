import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("Song")
    width: rootItem.width
    Menu {
        title: qsTr("Devices")
        Instantiator {
            model: deviceService.categories()
            Menu {
                id: categoryMenu
                title: modelData
                Instantiator {
                    model: deviceService.devicesByCategory(modelData)
                    MenuItem {
                        text: modelData
                        onTriggered: UiService.requestDeviceDialog(modelData)
                    }
                    onObjectAdded: (index, object) => categoryMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => categoryMenu.removeItem(object)
                }
            }
            onObjectAdded: (index, object) => insertMenu(index, object)
            onObjectRemoved: (index, object) => removeMenu(object)
        }
    }
    MenuSeparator {}
    Action {
        text: qsTr("Transpose <b>+1</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(1)
    }
    Action {
        text: qsTr("Transpose <b>-1</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(-1)
    }
    Action {
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(-2)
    }
    Action {
        text: qsTr("Transpose <b>+6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(6)
    }
    Action {
        text: qsTr("Transpose <b>-6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(-6)
    }
    Action {
        text: qsTr("Transpose <b>+12</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(12)
    }
    Action {
        text: qsTr("Transpose <b>-12</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestSongTranspose(-12)
    }
    delegate: MenuItemDelegate {}
}
