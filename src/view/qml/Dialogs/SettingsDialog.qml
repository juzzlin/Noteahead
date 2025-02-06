import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings") + "</strong>"
    modal: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                rootItem.accepted();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save current settings")
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        GroupBox {
            title: qsTr("General")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 1
                    width: parent.width
                    Label {
                        text: qsTr("Number of lines visible:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: visibleLinesSpinBox
                        from: 16
                        to: 32
                        Layout.column: 4
                        Layout.columnSpan: 5
                        Layout.row: 0
                        Layout.fillWidth: true
                        value: config.visibleLines
                        editable: true
                        onValueChanged: config.setVisibleLines(value)
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set number of visible lines in the editor view")
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
