import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."
import "../Components"

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
    MenuSeparator {}
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
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestTrackTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestTrackTranspose(-2)
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
    MenuSeparator {}
    Action {
        text: qsTr("Interpolate velocity")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestTrackVelocityInterpolationDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Insert a new track to the right")
        shortcut: "Shift+I"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestNewTrackToRight()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Delete current track")
        shortcut: "Shift+D"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestTrackDeletion()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit MIDI CC automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByTrack()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit Pitch Bend automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByTrack()
    }
    delegate: MenuItemDelegate {}
}
