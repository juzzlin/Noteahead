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
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
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
                            value: settingsService.visibleLines
                            editable: true
                            onValueChanged: settingsService.setVisibleLines(value)
                            Keys.onReturnPressed: focus = false
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Set number of visible lines in the editor view")
                        }
                    }
                }
            }
            GroupBox {
                height: parent.height
                width: parent.width
                title: qsTr("MIDI")
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    GroupBox {
                        title: qsTr("Controller")
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        GridLayout {
                            columns: 9
                            rows: 3
                            width: parent.width
                            Label {
                                text: qsTr("Port:")
                                Layout.column: 0
                                Layout.columnSpan: 2
                                Layout.row: 0
                                Layout.fillWidth: true
                            }
                            ComboBox {
                                id: portNameDropdown
                                model: midiSettingsModel.availableMidiPorts
                                currentIndex: 0
                                Layout.column: 2
                                Layout.columnSpan: 7
                                Layout.row: 0
                                Layout.fillWidth: true
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Set port of a MIDI controller keyboard")
                                function updateSelection() {
                                    const index = find(midiSettingsModel.controllerPort);
                                    if (index !== -1) {
                                        currentIndex = index;
                                    }
                                }
                                onCurrentTextChanged: midiSettingsModel.controllerPort = currentText
                                Connections {
                                    target: midiSettingsModel
                                    function onAvailableMidiPortsChanged() {
                                        portNameDropdown.updateSelection();
                                    }
                                }
                                Component.onCompleted: updateSelection()
                            }
                        }
                    }
                    GroupBox {
                        title: qsTr("Miscellaneous")
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
                                value: settingsService.autoNoteOffOffset()
                                editable: true
                                onValueChanged: settingsService.setAutoNoteOffOffset(value)
                                Keys.onReturnPressed: focus = false
                                ToolTip.delay: Constants.toolTipDelay
                                ToolTip.timeout: Constants.toolTipTimeout
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Set offset for auto note-off events in milliseconds")
                            }
                        }
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
