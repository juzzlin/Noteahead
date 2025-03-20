import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`
    GridLayout {
        anchors.fill: parent
        SpinBox {
            id: controllerSpinBox
            from: 0
            to: 127
            editable: true
            value: model.controller
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Controller")
            onValueChanged: {
                model.controller = value;
            }
            onActiveFocusChanged: editing = activeFocus
        }
        SpinBox {
            id: startValueSpinBox
            from: 0
            to: 127
            value: model.value0
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start value")
            onValueChanged: model.value0 = value
        }
        SpinBox {
            id: endValueSpinBox
            from: 0
            to: 127
            value: model.value1
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End value")
            onValueChanged: model.value1 = value
        }
        SpinBox {
            id: startLineSpinBox
            from: 0
            to: endLineSpinBox.value
            value: model.line0
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 3
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start line")
            onValueChanged: model.line0 = value
        }
        SpinBox {
            id: endLineSpinBox
            from: startLineSpinBox.value + 1
            to: 999
            value: model.line1
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 4
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End line")
            onValueChanged: model.line1 = value
        }
        TextField {
            id: commentEdit
            readOnly: false
            text: model.comment
            placeholderText: qsTr("Comment")
            Layout.row: 2
            Layout.columnSpan: 5
            Layout.fillWidth: true
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Comment")
            onTextChanged: model.comment = text
        }
    }
}
