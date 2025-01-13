import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs

Dialog {
    id: rootItem
    title: qsTr("Save unsaved changes?")
    modal: true
    standardButtons: DialogButtonBox.Yes | DialogButtonBox.Discard | DialogButtonBox.Cancel
    Component.onCompleted: {
        visible = false;
    }
}
