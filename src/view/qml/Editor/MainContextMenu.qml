import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    id: rootItem

    readonly property string trackPortName: editorService.instrumentPortName(editorService.position.track)
    readonly property bool trackHasInternalDevice: deviceService.isInternalDevice(trackPortName)

    MenuItem {
        text: qsTr("Insert FX...")
        visible: trackHasInternalDevice
        onTriggered: UiService.requestDeviceInsertEffectsDialog(trackPortName)
    }
    MenuSeparator {
        visible: trackHasInternalDevice
    }
    MenuItem {
        text: qsTr("Effect Sends...")
        visible: trackHasInternalDevice
        onTriggered: UiService.requestEffectSendsDialog(trackPortName)
    }
    MenuSeparator {
        visible: trackHasInternalDevice
    }
    MainContextMenu_Line {}
    MenuSeparator {}
    MainContextMenu_Column {}
    MenuSeparator {}
    MainContextMenu_Track {}
    MenuSeparator {}
    MainContextMenu_Pattern {}
    MenuSeparator {}
    MainContextMenu_Selection {}
    MenuSeparator {}
    MainContextMenu_Song {}
    MenuSeparator {}
    Action {
        text: qsTr("Edit MIDI CC automations (ALL)")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit Pitch Bend automations (ALL)")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialog()
    }
    delegate: MenuItemDelegate {}
}
