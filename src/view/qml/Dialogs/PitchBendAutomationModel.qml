import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts

Item {
    id: rootItem
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
    function comment() {
        return commentEdit.text;
    }
    function setComment(comment) {
        commentEdit.text = comment;
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        GroupBox {
            title: qsTr("Interpolation settings")
            Layout.fillWidth: true
            GridLayout {
                rowSpacing: 10
                width: parent.width
                Label {
                    text: qsTr("Start line")
                    Layout.row: 0
                    Layout.column: 0
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("End line")
                    Layout.row: 0
                    Layout.column: 1
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Start value (%)")
                    Layout.row: 0
                    Layout.column: 2
                }
                SpinBox {
                    id: startValueSpinBox
                    from: -100
                    to: 100
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                Label {
                    text: " " + qsTr("End value (%)")
                    Layout.row: 0
                    Layout.column: 3
                }
                SpinBox {
                    id: endValueSpinBox
                    from: -100
                    to: 100
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                }
            }
        }
        GroupBox {
            title: qsTr("Comment")
            Layout.fillWidth: true
            Layout.row: 2
            Layout.column: 0
            TextField {
                id: commentEdit
                readOnly: false
                width: parent.width
                Keys.onReturnPressed: {
                    focus = false;
                }
            }
        }
    }
}
