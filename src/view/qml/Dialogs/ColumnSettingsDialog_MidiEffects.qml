import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

ColumnLayout {
    id: rootItem
    spacing: 10
    width: parent.width
    function initialize() {
        midiDelaySpinBox.value = columnSettingsModel.midiDelayLines * midiDelaySpinBox.factor
        midiDelayFeedbackSpinBox.value = columnSettingsModel.midiDelayFeedback
        midiDelayMaxRepetitionsSpinBox.value = columnSettingsModel.midiDelayMaxRepetitions
    }
    GroupBox {
        title: qsTr("Chord Automation")
        Layout.fillWidth: true
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
                    value: columnSettingsModel.chordNote1Offset
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
                    value: columnSettingsModel.chordNote1Velocity
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
                    value: columnSettingsModel.chordNote1Delay
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
                    value: columnSettingsModel.chordNote2Offset
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
                    value: columnSettingsModel.chordNote2Velocity
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
                    value: columnSettingsModel.chordNote2Delay
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
                    value: columnSettingsModel.chordNote3Offset
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
                    value: columnSettingsModel.chordNote3Velocity
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
                    value: columnSettingsModel.chordNote3Delay
                    editable: true
                    Keys.onReturnPressed: focus = false
                    onValueModified: columnSettingsModel.chordNote3Delay = value
                    Layout.row: 2
                    Layout.column: 5
                }
            }
            Flow {
                Layout.fillWidth: true
                spacing: 4
                Button {
                    text: qsTr("Major")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Major chord (0, 4, 7)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 4;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 0;
                    }
                }
                Button {
                    text: qsTr("Minor")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Minor chord (0, 3, 7)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 3;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 0;
                    }
                }
                Button {
                    text: qsTr("Dom7")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Dominant 7th chord (0, 4, 7, 10)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 4;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 10;
                    }
                }
                Button {
                    text: qsTr("Maj7")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Major 7th chord (0, 4, 7, 11)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 4;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 11;
                    }
                }
                Button {
                    text: qsTr("Min7")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Minor 7th chord (0, 3, 7, 10)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 3;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 10;
                    }
                }
                Button {
                    text: qsTr("Sus4")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Suspended 4th chord (0, 5, 7)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 5;
                        columnSettingsModel.chordNote2Offset = 7;
                        columnSettingsModel.chordNote3Offset = 0;
                    }
                }
                Button {
                    text: qsTr("Dim")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Diminished chord (0, 3, 6)")
                    onClicked: {
                        columnSettingsModel.chordNote1Offset = 3;
                        columnSettingsModel.chordNote2Offset = 6;
                        columnSettingsModel.chordNote3Offset = 0;
                    }
                }
                Button {
                    text: qsTr("Reset")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reset chord automation settings")
                    onClicked: columnSettingsModel.reset()
                }
            }
            LayoutSeparator {
                Layout.fillWidth: true
            }
            GridLayout {
                columns: 6
                width: parent.width
                Layout.fillWidth: true
                CheckBox {
                    id: arpeggiatorEnabledCheckBox
                    text: qsTr("Enable Arpeggiator")
                    checked: columnSettingsModel.arpeggiatorEnabled
                    onCheckedChanged: columnSettingsModel.arpeggiatorEnabled = checked
                    Layout.columnSpan: 2
                }
                Label {
                    text: qsTr("Pattern:")
                    enabled: arpeggiatorEnabledCheckBox.checked
                }
                ComboBox {
                    id: arpeggiatorPatternComboBox
                    model: [qsTr("Up"), qsTr("Down"), qsTr("Up-Down"), qsTr("Down-Up"), qsTr("Random")]
                    currentIndex: columnSettingsModel.arpeggiatorPattern
                    enabled: arpeggiatorEnabledCheckBox.checked
                    onActivated: index => columnSettingsModel.arpeggiatorPattern = index
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Events per beat:")
                    enabled: arpeggiatorEnabledCheckBox.checked
                }
                SpinBox {
                    id: arpeggiatorEventsPerBeatSpinBox
                    from: 1
                    to: 32
                    value: columnSettingsModel.arpeggiatorEventsPerBeat
                    enabled: arpeggiatorEnabledCheckBox.checked
                    editable: true
                    Keys.onReturnPressed: focus = false
                    onValueModified: columnSettingsModel.arpeggiatorEventsPerBeat = value
                    Layout.fillWidth: true
                }
            }
        }
    }
    GroupBox {
        title: qsTr("MIDI Delay")
        Layout.fillWidth: true
        RowLayout {
            spacing: 10
            Label {
                text: qsTr("Lines:")
            }
            SpinBox {
                id: midiDelaySpinBox
                readonly property int decimals: 2
                readonly property int factor: Math.pow(10, decimals)
                from: 0
                to: 64 * factor
                value: columnSettingsModel.midiDelayLines * factor
                stepSize: 0.25 * factor
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.midiDelayLines = value / factor
                Layout.fillWidth: false

                validator: DoubleValidator {
                    bottom: midiDelaySpinBox.from / midiDelaySpinBox.factor
                    top: midiDelaySpinBox.to / midiDelaySpinBox.factor
                }

                textFromValue: function(value, locale) {
                    return Number(value / factor).toLocaleString(locale, 'f', decimals)
                }

                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text) * factor
                }
            }
            Label {
                text: qsTr("Feedback (%):")
            }
            SpinBox {
                id: midiDelayFeedbackSpinBox
                from: 0
                to: 100
                value: columnSettingsModel.midiDelayFeedback
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.midiDelayFeedback = value
                Layout.fillWidth: false
            }
            Label {
                text: qsTr("Max repetitions:")
            }
            SpinBox {
                id: midiDelayMaxRepetitionsSpinBox
                from: 1
                to: 64
                value: columnSettingsModel.midiDelayMaxRepetitions
                editable: true
                Keys.onReturnPressed: focus = false
                onValueModified: columnSettingsModel.midiDelayMaxRepetitions = value
                Layout.fillWidth: false
            }
        }
    }
}
