import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Chord Automation")
    Layout.fillWidth: true
    width: parent.width
    clip: true
    function initialize() {
        note1OffsetSpinBox.value = columnSettingsModel.chordNote1Offset;
        note1VelocitySpinBox.value = columnSettingsModel.chordNote1Velocity;
        note1DelaySpinBox.value = columnSettingsModel.chordNote1Delay;
        note2OffsetSpinBox.value = columnSettingsModel.chordNote2Offset;
        note2VelocitySpinBox.value = columnSettingsModel.chordNote2Velocity;
        note2DelaySpinBox.value = columnSettingsModel.chordNote2Delay;
        note3OffsetSpinBox.value = columnSettingsModel.chordNote3Offset;
        note3VelocitySpinBox.value = columnSettingsModel.chordNote3Velocity;
        note3DelaySpinBox.value = columnSettingsModel.chordNote3Delay;
    }
    ColumnLayout {
        spacing: 8
        Layout.fillWidth: true
        GridLayout {
            width: parent.width
            Layout.fillWidth: true
            Label {
                text: qsTr("Note 1 Offset:")
                Layout.row: 0
                Layout.column: 0
            }
            SpinBox {
                id: note1OffsetSpinBox
                from: -36
                to: 36
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote1Offset = value
                Layout.row: 0
                Layout.column: 1
                Layout.rightMargin: 10
            }
            Label {
                text: qsTr("Velocity (%):")
                Layout.row: 0
                Layout.column: 2
            }
            SpinBox {
                id: note1VelocitySpinBox
                from: 0
                to: 200
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote1Velocity = value
                Layout.row: 0
                Layout.column: 3
                Layout.rightMargin: 10
            }
            Label {
                text: qsTr("Delay (ms):")
                Layout.row: 0
                Layout.column: 4
            }
            SpinBox {
                id: note1DelaySpinBox
                from: 0
                to: 1000
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote1Delay = value
                Layout.row: 0
                Layout.column: 5
            }
            Label {
                text: qsTr("Note 2 Offset:")
                Layout.row: 1
                Layout.column: 0
            }
            SpinBox {
                id: note2OffsetSpinBox
                from: -36
                to: 36
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote2Offset = value
                Layout.row: 1
                Layout.column: 1
            }
            Label {
                text: qsTr("Velocity (%):")
                Layout.row: 1
                Layout.column: 2
            }
            SpinBox {
                id: note2VelocitySpinBox
                from: 0
                to: 200
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote2Velocity = value
                Layout.row: 1
                Layout.column: 3
                Layout.rightMargin: 10
            }
            Label {
                text: qsTr("Delay (ms):")
                Layout.row: 1
                Layout.column: 4
            }
            SpinBox {
                id: note2DelaySpinBox
                from: 0
                to: 1000
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote2Delay = value
                Layout.row: 1
                Layout.column: 5
            }
            Label {
                text: qsTr("Note 3 Offset:")
                Layout.row: 2
                Layout.column: 0
            }
            SpinBox {
                id: note3OffsetSpinBox
                from: -36
                to: 36
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote3Offset = value
                Layout.row: 2
                Layout.column: 1
            }
            Label {
                text: qsTr("Velocity (%):")
                Layout.row: 2
                Layout.column: 2
            }
            SpinBox {
                id: note3VelocitySpinBox
                from: 0
                to: 200
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote3Velocity = value
                Layout.row: 2
                Layout.column: 3
                Layout.rightMargin: 10
            }
            Label {
                text: qsTr("Delay (ms):")
                Layout.row: 2
                Layout.column: 4
            }
            SpinBox {
                id: note3DelaySpinBox
                from: 0
                to: 1000
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.chordNote3Delay = value
                Layout.row: 2
                Layout.column: 5
            }
        }
    }
}
