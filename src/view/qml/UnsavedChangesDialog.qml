import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import QtQuick.Dialogs

Dialog {
    id: rootItem
    title: qsTr("Save unsaved changes?")
    modal: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Yes")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: rootItem.accepted()
        }
        Button {
            text: qsTr("Close without saving")
            DialogButtonBox.buttonRole: DialogButtonBox.NoRole
            onClicked: rootItem.discarded()
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
        }
    }
}
