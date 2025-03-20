import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts

Item {
    id: rootItem
    function controller() {
        return midiCcSelector.controller();
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
            title: qsTr("Controller number")
            Layout.fillWidth: true
            MidiCcSelector {
                id: midiCcSelector
                Component.onCompleted: {
                    showEnabled = false;
                    showValue = false;
                    setEnabled(true);
                }
            }
        }
        GroupBox {
            title: qsTr("Value and line ranges")
            Layout.fillWidth: true
            GridLayout {
                rowSpacing: 10
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
                    to: endLineSpinBox.value
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
                    from: startLineSpinBox.value + 1
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
        GroupBox {
            title: qsTr("Comment")
            Layout.fillWidth: true
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
