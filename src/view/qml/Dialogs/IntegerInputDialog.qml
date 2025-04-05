import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function setTitle(text) {
        title = "<strong>" + text + "</strong>";
    }
    function setMinValue(value) {
        spinBox.from = value;
    }
    function setMaxValue(value) {
        spinBox.to = value;
    }
    function setValue(value) {
        spinBox.value = value;
    }
    function value() {
        return spinBox.value;
    }
    contentItem: RowLayout {
        spacing: 10
        width: parent.width
        Label {
            text: qsTr("Choose a value (%1-%2):").arg(spinBox.from).arg(spinBox.to)
            width: parent.width
        }
        SpinBox {
            id: spinBox
            width: parent.width * 0.6
            from: 0
            to: 100
            value: 50
            editable: true
            Keys.onReturnPressed: {
                focus = false;
                rootItem.accept();
            }
            Layout.fillWidth: true
        }
    }
}
