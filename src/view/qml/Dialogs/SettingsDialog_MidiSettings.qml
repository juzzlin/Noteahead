import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

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
                    model: midiSettingsModel.midiInPorts
                    currentIndex: 0
                    Layout.column: 2
                    Layout.columnSpan: 7
                    Layout.row: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set the port of a MIDI controller keyboard")
                    function updateSelection() {
                        const index = find(midiSettingsModel.controllerPort);
                        if (index !== -1) {
                            currentIndex = index;
                        }
                    }
                    onCurrentTextChanged: midiSettingsModel.controllerPort = currentText
                    Connections {
                        target: midiSettingsModel
                        function onMidiInPortsChanged() {
                            portNameDropdown.updateSelection();
                        }
                    }
                    Component.onCompleted: updateSelection()
                }
                Label {
                    text: qsTr("Data:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 1
                    Layout.fillWidth: true
                }
                TextField {
                    text: midiSettingsModel.debugData
                    readOnly: true
                    Layout.column: 2
                    Layout.columnSpan: 7
                    Layout.row: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Received MIDI data")
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
