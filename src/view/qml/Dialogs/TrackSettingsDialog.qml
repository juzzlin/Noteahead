import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings for track") + ` '${editorService.trackName(_trackIndex)}'` + "</strong>"
    modal: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: rootItem.accepted()
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
        }
    }
    property int _trackIndex: 0
    function setTrackIndex(trackIndex) {
        _trackIndex = trackIndex;
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        GroupBox {
            title: qsTr("MIDI Settings")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 2
                    width: parent.width
                    Label {
                        text: qsTr("Port:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        id: midiPortDropdown
                        model: UiService.requestAvailableMidiPorts()
                        currentIndex: 0
                        Layout.column: 2
                        Layout.columnSpan: 7
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Channel:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 1
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        id: midiChannelDropdown
                        model: ListModel {
                            // Populate channels 1–16
                            Component.onCompleted: {
                                for (let i = 1; i <= 16; i++)
                                    append({
                                        "channel": i
                                    });
                            }
                        }
                        textRole: "channel"
                        currentIndex: 0
                        Layout.column: 2
                        Layout.columnSpan: 7
                        Layout.row: 1
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Patch:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enablePatchCheckbox
                        text: qsTr("Enable Patch")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: midiPatchSpinBox
                        from: 0
                        to: 127
                        enabled: enablePatchCheckbox.checked
                        Layout.column: 4
                        Layout.columnSpan: 5
                        Layout.row: 2
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enableBankCheckbox
                        text: qsTr("Enable Bank")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Bank (LSB):")
                        Layout.column: 2
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: bankLsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 3
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Bank (MSB):")
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: bankMsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 6
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: swapBankByteOrderCheckBox
                        text: qsTr("Swap LSB/MSB")
                        enabled: enableBankCheckbox.checked
                        Layout.column: 7
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
