import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."
import "../Components"

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
    MenuSeparator {}
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
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(-2)
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
    MenuSeparator {}
    Action {
        text: qsTr("Interpolate velocity")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnVelocityInterpolationDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add MIDI CC automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnAddMidiCcAutomationDialog()
    }
    Action {
        text: qsTr("Edit MIDI CC automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByColumn()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Pitch Bend automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnAddPitchBendAutomationDialog()
    }
    Action {
        text: qsTr("Edit Pitch Bend automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByColumn()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add instrument layer")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnAddInstrumentLayerDialog()
    }
    Action {
        text: qsTr("Edit instrument layers")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnEditInstrumentLayersDialog()
    }
    delegate: MenuItemDelegate {}
}
