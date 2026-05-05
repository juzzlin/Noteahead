import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    modal: true
    anchors.centerIn: parent
    width: parent.width * 0.4
    standardButtons: Dialog.Ok | Dialog.Cancel
    property alias text: textField.text
    function setTitle(titleText) {
        title = "<strong>" + titleText + "</strong>";
    }
    contentItem: RowLayout {
        spacing: 10
        width: parent.width
        Label {
            text: qsTr("Name:")
        }
        TextField {
            id: textField
            width: parent.width * 0.8
            focus: true
            selectByMouse: true
            Keys.onReturnPressed: {
                focus = false;
                rootItem.accept();
            }
            Layout.fillWidth: true
        }
    }
    onOpened: textField.forceActiveFocus()
}
