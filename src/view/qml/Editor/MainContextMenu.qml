import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."
import "../Components"

Menu {
    id: rootItem
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
