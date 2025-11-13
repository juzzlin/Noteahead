import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function usePercentages() {
        return percentageCheckBox.checked;
    }
    function setTitle(text) {
        title = "<strong>" + text + "</strong>";
    }
    function startValue() {
        return startValueSpinBox.value;
    }
    function setStartValue(value) {
        startValueSpinBox.value = value;
    }
    function endValue(value) {
        return endValueSpinBox.value;
    }
    function setEndValue(value) {
        endValueSpinBox.value = value;
    }
    function startLine() {
        return startLineSpinBox.value;
    }
    function setStartLine(value) {
        startLineSpinBox.value = value;
    }
    function endLine() {
        return endLineSpinBox.value;
    }
    function setEndLine(value) {
        endLineSpinBox.value = value;
    }
    contentItem: GridLayout {
        rowSpacing: 10
        width: parent.width
        CheckBox {
            id: percentageCheckBox
            text: qsTr("Use percentages")
            Layout.columnSpan: 4
            Layout.row: 0
            onCheckedChanged: {
                if (checked) {
                    startValueSpinBox.from = 0;
                    startValueSpinBox.to = 200;
                    endValueSpinBox.from = 0;
                    endValueSpinBox.to = 200;
                    if (startValueSpinBox.value > 200) startValueSpinBox.value = 200;
                    if (endValueSpinBox.value > 200) endValueSpinBox.value = 200;
                } else {
                    startValueSpinBox.from = 0;
                    startValueSpinBox.to = 127;
                    endValueSpinBox.from = 0;
                    endValueSpinBox.to = 127;
                }
            }
        }
        Label {
            text: qsTr("Start value:")
            Layout.column: 0
            Layout.row: 1
        }
        SpinBox {
            id: startValueSpinBox
            width: parent.width * 0.3
            from: 0
            to: 127
            value: 0
            editable: true
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.column: 1
            Layout.row: 1
            Layout.fillWidth: true
        }
        Label {
            text: " " + qsTr("End value:")
            Layout.column: 2
            Layout.row: 1
        }
        SpinBox {
            id: endValueSpinBox
            width: parent.width * 0.3
            from: 0
            to: 127
            value: 0
            editable: true
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.column: 3
            Layout.row: 1
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Start line:")
            width: parent.width
            Layout.column: 0
            Layout.row: 2
        }
        SpinBox {
            id: startLineSpinBox
            width: parent.width * 0.3
            from: 0
            to: 999
            value: 0
            editable: true
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.column: 1
            Layout.row: 2
            Layout.fillWidth: true
        }
        Label {
            text: " " + qsTr("End line:")
            Layout.column: 2
            Layout.row: 2
        }
        SpinBox {
            id: endLineSpinBox
            width: parent.width * 0.3
            from: 0
            to: 999
            value: 0
            editable: true
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.column: 3
            Layout.row: 2
            Layout.fillWidth: true
        }
    }
}
