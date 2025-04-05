import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Menu {
    id: rootItem
    Menu {
        title: qsTr("Line")
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Set delay")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestLineDelayDialog()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: qsTr("Column")
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
            text: qsTr("Transpose <b>+6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(6)
        }
        Action {
            text: qsTr("Transpose <b>-6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestColumnTranspose(-6)
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Interpolate velocity")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestColumnVelocityInterpolationDialog()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    MenuSeparator {
    }
    Menu {
        title: qsTr("Track")
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
            text: qsTr("Transpose <b>+6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(6)
        }
        Action {
            text: qsTr("Transpose <b>-6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackTranspose(-6)
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
        MenuSeparator {
        }
        Action {
            text: qsTr("Insert a new track to the right")
            shortcut: "Shift+I"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestNewTrackToRight()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Delete current track")
            shortcut: "Shift+D"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestTrackDeletion()
        }
    }
    MenuSeparator {
    }
    Menu {
        title: "Pattern"
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
            text: qsTr("Transpose <b>+6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(6)
        }
        Action {
            text: qsTr("Transpose <b>-6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-6)
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
    MenuSeparator {
    }
    Menu {
        title: qsTr("Selection")
        width: rootItem.width
        Action {
            text: qsTr("Cut")
            shortcut: "Ctrl+X"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestSelectionCut()
        }
        Action {
            text: qsTr("Copy")
            shortcut: "Ctrl+C"
            onTriggered: editorService.requestSelectionCopy()
        }
        Action {
            text: qsTr("Paste")
            shortcut: "Ctrl+V"
            enabled: !UiService.isPlaying() && editorService.hasSelectionToPaste
            onTriggered: editorService.requestSelectionPaste()
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Transpose <b>+1</b> semitones")
            shortcut: "Alt+Shift+F10"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestSelectionTranspose(1)
        }
        Action {
            text: qsTr("Transpose <b>-1</b> semitones")
            shortcut: "Alt+Shift+F9"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestSelectionTranspose(-1)
        }
        Action {
            text: qsTr("Transpose <b>+12</b> semitones")
            shortcut: "Alt+Shift+F12"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestSelectionTranspose(12)
        }
        Action {
            text: qsTr("Transpose <b>-12</b> semitones")
            shortcut: "Alt+Shift+F11"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestSelectionTranspose(-12)
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Interpolate velocity")
            enabled: !UiService.isPlaying() && selectionService.isValidSelection
            onTriggered: UiService.requestSelectionVelocityInterpolationDialog()
        }
        delegate: MainMenuItemDelegate {
        }
    }
    delegate: MainMenuItemDelegate {
    }
}
