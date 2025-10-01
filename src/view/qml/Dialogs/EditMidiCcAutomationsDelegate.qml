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
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable/disable the automation")
            onCheckedChanged: model.enabled = checked
            Component.onCompleted: checked = model.enabled
        }
        Label {
            text: qsTr("Controller")
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
        }
        SpinBox {
            id: controllerSpinBox
            from: 0
            to: 127
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Controller")
            onValueChanged: model.controller = value
            onActiveFocusChanged: editing = activeFocus
            Component.onCompleted: value = model.controller
        }
        Label {
            text: qsTr("Start line")
            Layout.row: 0
            Layout.column: 2
            Layout.fillWidth: true
        }
        SpinBox {
            id: startLineSpinBox
            from: 0
            to: 999
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start line")
            onValueChanged: model.line0 = value
            Component.onCompleted: value = model.line0
        }
        Label {
            text: qsTr("End line")
            Layout.row: 0
            Layout.column: 3
            Layout.fillWidth: true
        }
        SpinBox {
            id: endLineSpinBox
            from: 0
            to: 999
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 3
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End line")
            onValueChanged: model.line1 = value
            Component.onCompleted: value = model.line1
        }
        Label {
            text: qsTr("Start value")
            Layout.row: 0
            Layout.column: 4
            Layout.fillWidth: true
        }
        SpinBox {
            id: startValueSpinBox
            from: 0
            to: 127
            editable: true
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 4
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Start value")
            onValueChanged: model.value0 = value
            Component.onCompleted: value = model.value0
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
            editable: true
            enabled: model.line0 !== model.line1
            Keys.onReturnPressed: focus = false
            Layout.row: 1
            Layout.column: 5
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("End value")
            onValueChanged: model.value1 = value
            Component.onCompleted: value = model.value1
        }
        Button {
            id: rootItem
            Layout.row: 1
            Layout.rowSpan: 2
            Layout.column: 6
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
            onClicked: midiCcAutomationsModel.removeAt(index)
        }
        TextField {
            id: commentEdit
            readOnly: false
            placeholderText: qsTr("Comment")
            Layout.row: 2
            Layout.columnSpan: 6
            Layout.fillWidth: true
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Comment")
            onTextChanged: model.comment = text
            Component.onCompleted: text = model.comment
        }
    }
}
