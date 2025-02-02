import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Menu {
    id: rootItem
    Action {
        text: qsTr("Cut column")
        shortcut: "Alt+F3"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnCut()
    }
    Action {
        text: qsTr("Copy column")
        shortcut: "Alt+F4"
        onTriggered: editorService.requestColumnCopy()
    }
    Action {
        text: qsTr("Paste column")
        shortcut: "Alt+F5"
        enabled: !UiService.isPlaying() && editorService.hasColumnToPaste
        onTriggered: editorService.requestColumnPaste()
    }
    MenuSeparator {
    }
    Action {
        text: qsTr("Cut track")
        shortcut: "Shift+F3"
        enabled: UiService.isPlaying()
        onTriggered: editorService.requestTrackCut()
    }
    Action {
        text: qsTr("Copy track")
        shortcut: "Shift+F4"
        onTriggered: editorService.requestTrackCopy()
    }
    Action {
        text: qsTr("Paste track")
        shortcut: "Shift+F5"
        enabled: !UiService.isPlaying() && editorService.hasTrackToPaste
        onTriggered: editorService.requestTrackPaste()
    }
    MenuSeparator {
    }
    Action {
        text: qsTr("Cut pattern")
        shortcut: "Ctrl+F3"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternCut()
    }
    Action {
        text: qsTr("Copy pattern")
        shortcut: "Ctrl+F4"
        onTriggered: editorService.requestPatternCopy()
    }
    Action {
        text: qsTr("Paste pattern")
        shortcut: "Ctrl+F5"
        enabled: !UiService.isPlaying() && editorService.hasPatternToPaste
        onTriggered: editorService.requestPatternPaste()
    }
    delegate: MainMenuItemDelegate {
    }
}
