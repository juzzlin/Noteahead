import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("Song")
    width: rootItem.width
    Action {
        text: qsTr("Device Rack...")
        onTriggered: applicationService.requestDeviceRackDialog()
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
