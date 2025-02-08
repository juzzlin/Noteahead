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
    Column {
        anchors.fill: parent
        spacing: 10
        StackLayout {
            id: mainLayout
            height: parent.height - tabBar.height
            width: parent.width
            currentIndex: tabBar.currentIndex
            GroupBox {
                height: parent.height
                width: parent.width
                title: qsTr("General")
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
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set number of visible lines in the editor view")
                    }
                }
            }
            GroupBox {
                height: parent.height
                width: parent.width
                title: qsTr("MIDI")
                GridLayout {
                    columns: 9
                    rows: 1
                    width: parent.width
                    Label {
                        text: qsTr("Auto note-off offset (ms):")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: autoNoteOffOffsetSpinBox
                        from: 0
                        to: 500
                        Layout.column: 4
                        Layout.columnSpan: 5
                        Layout.row: 0
                        Layout.fillWidth: true
                        value: config.autoNoteOffOffset()
                        editable: true
                        onValueChanged: config.setAutoNoteOffOffset(value)
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set offset for auto note-off events in milliseconds")
                    }
                }
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("General")
            }
            TabButton {
                text: qsTr("MIDI")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
