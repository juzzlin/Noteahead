import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Menu {
    id: rootItem
    Menu {
        title: qsTr("Column")
        Action {
            text: qsTr("Cut")
            shortcut: "Alt+F3"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnCut()
        }
        Action {
            text: qsTr("Copy")
            shortcut: "Alt+F4"
            onTriggered: editorService.requestColumnCopy()
        }
        Action {
            text: qsTr("Paste")
            shortcut: "Alt+F5"
            enabled: !UiService.isPlaying() && editorService.hasColumnToPaste
            onTriggered: editorService.requestColumnPaste()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: qsTr("Track")
        Action {
            text: qsTr("Cut")
            shortcut: "Shift+F3"
            enabled: UiService.isPlaying()
            onTriggered: editorService.requestTrackCut()
        }
        Action {
            text: qsTr("Copy")
            shortcut: "Shift+F4"
            onTriggered: editorService.requestTrackCopy()
        }
        Action {
            text: qsTr("Paste")
            shortcut: "Shift+F5"
            enabled: !UiService.isPlaying() && editorService.hasTrackToPaste
            onTriggered: editorService.requestTrackPaste()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: "Pattern"
        Action {
            text: qsTr("Cut")
            shortcut: "Ctrl+F3"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternCut()
        }
        Action {
            text: qsTr("Copy")
            shortcut: "Ctrl+F4"
            onTriggered: editorService.requestPatternCopy()
        }
        Action {
            text: qsTr("Paste")
            shortcut: "Ctrl+F5"
            enabled: !UiService.isPlaying() && editorService.hasPatternToPaste
            onTriggered: editorService.requestPatternPaste()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    delegate: MainMenuItemDelegate {
    }
}
