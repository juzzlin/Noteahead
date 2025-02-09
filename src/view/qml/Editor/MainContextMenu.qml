import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Menu {
    id: rootItem
    Menu {
        title: qsTr("Line")
        height: rootItem.height
        width: rootItem.width
        Action {
            text: qsTr("Insert an event")
            shortcut: "Alt+E"
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEventSelectionDialog()
        }
        Action {
            text: qsTr("Remove current event")
            shortcut: "Alt+Shift+E"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestEventRemoval()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: qsTr("Column")
        height: rootItem.height
        width: rootItem.width
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Transpose <b>+1</b> semitones")
            shortcut: "Alt+F10"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(1)
        }
        Action {
            text: qsTr("Transpose <b>-1</b> semitones")
            shortcut: "Alt+F9"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(-1)
        }
        Action {
            text: qsTr("Transpose <b>+12</b> semitones")
            shortcut: "Alt+F12"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(12)
        }
        Action {
            text: qsTr("Transpose <b>-12</b> semitones")
            shortcut: "Alt+F11"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(-12)
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: qsTr("Track")
        height: rootItem.height
        width: rootItem.width
        Action {
            text: qsTr("Cut")
            shortcut: "Shift+F3"
            enabled: !UiService.isPlaying()
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Transpose <b>+1</b> semitones")
            shortcut: "Shift+F10"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(1)
        }
        Action {
            text: qsTr("Transpose <b>-1</b> semitones")
            shortcut: "Shift+F9"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(-1)
        }
        Action {
            text: qsTr("Transpose <b>+12</b> semitones")
            shortcut: "Shift+F12"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(12)
        }
        Action {
            text: qsTr("Transpose <b>-12</b> semitones")
            shortcut: "Shift+F11"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(-12)
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: "Pattern"
        height: rootItem.height
        width: rootItem.width
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Transpose <b>+1</b> semitones")
            shortcut: "Ctrl+F10"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(1)
        }
        Action {
            text: qsTr("Transpose <b>-1</b> semitones")
            shortcut: "Ctrl+F9"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-1)
        }
        Action {
            text: qsTr("Transpose <b>+12</b> semitones")
            shortcut: "Ctrl+F12"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(12)
        }
        Action {
            text: qsTr("Transpose <b>-12</b> semitones")
            shortcut: "Ctrl+F11"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-12)
        }
        delegate: MainMenuItemDelegate {
        }
    }
    delegate: MainMenuItemDelegate {
    }
}
