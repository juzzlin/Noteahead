import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../ToolBar"

GroupBox {
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`
    GridLayout {
        anchors.fill: parent
        CheckBox {
            id: enableCheckbox
            text: qsTr("Enabled")
            checked: model.enabled
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable/disable the automation")
            onCheckedChanged: model.enabled = checked
        }
        Label {
            text: qsTr("Start line")
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
        }
        SpinBox {
            id: startLineSpinBox
            from: 0
            to: 999
            value: model.line0
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start line")
            onValueChanged: model.line0 = value
        }
        Label {
            text: qsTr("End line")
            Layout.row: 0
            Layout.column: 2
            Layout.fillWidth: true
        }
        SpinBox {
            id: endLineSpinBox
            from: 0
            to: 999
            value: model.line1
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End line")
            onValueChanged: model.line1 = value
        }
        Label {
            text: qsTr("Start value")
            Layout.row: 0
            Layout.column: 3
            Layout.fillWidth: true
        }
        SpinBox {
            id: startValueSpinBox
            from: 0
            to: 127
            value: model.value0
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 3
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start value")
            onValueChanged: model.value0 = value
        }
        Label {
            text: qsTr("End value")
            Layout.row: 0
            Layout.column: 5
            Layout.fillWidth: true
        }
        SpinBox {
            id: endValueSpinBox
            from: 0
            to: 127
            value: model.value1
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 4
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End value")
            onValueChanged: model.value1 = value
        }
        Button {
            id: rootItem
            Layout.row: 1
            Layout.rowSpan: 2
            Layout.column: 5
            Layout.columnSpan: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete")
            Image {
                id: backgroundImage
                source: "../Graphics/delete.svg"
                sourceSize: Qt.size(parent.width, parent.height)
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
            }
            onClicked: pitchBendAutomationsModel.removeAt(index)
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
