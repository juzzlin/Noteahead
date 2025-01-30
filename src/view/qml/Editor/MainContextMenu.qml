import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Menu {
    id: rootItem
    Action {
        text: qsTr("Copy pattern")
        shortcut: "Ctrl+F4"
        onTriggered: editorService.requestPatternCopy()
    }
    Action {
        text: qsTr("Paste pattern")
        shortcut: "Ctrl+F5"
        onTriggered: editorService.requestPatternPaste()
    }
    delegate: MainMenuItemDelegate {
    }
}
