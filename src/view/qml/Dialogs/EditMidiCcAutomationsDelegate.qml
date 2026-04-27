import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"
import "../ToolBar"

GroupBox {
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`
    function initialize(): void {
        if (model && model.controller !== undefined) {
            controllerComboBox.currentIndex = controllerComboBox.indexOfValue(model.controller);
            startLineSpinBox.value = model.line0;
            endLineSpinBox.value = model.line1;
            startValueSpinBox.value = model.value0;
            endValueSpinBox.value = model.value1;
            eventsPerBeatSpinBox.value = model.eventsPerBeat;
            lineOffsetSpinBox.value = model.lineOffset;
            modulationTypeComboBox.currentIndex = model.modulationType;
            modulationCyclesSpinBox.value = model.modulationCycles;
            modulationAmplitudeSpinBox.value = model.modulationAmplitude;
            modulationOffsetSpinBox.value = model.modulationOffset;
            modulationInvertedCheckBox.checked = model.modulationInverted;
        } else {
            // Fallback if model.controller is undefined (should not happen in practice if model is valid)
            controllerComboBox.currentIndex = 0;
        }
    }
    GridLayout {
        anchors.fill: parent
        columns: 10
        rowSpacing: 10
        CheckBox {
            id: enableCheckbox
            text: qsTr("Enabled")
            Layout.row: 0
            Layout.column: 0
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable/disable the automation")
            onCheckedChanged: model.enabled = checked
            Component.onCompleted: checked = model.enabled
        }
        GroupBox {
            title: qsTr("Interpolation")
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                Label {
                    text: qsTr("Controller")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                MidiCcComboBox {
                    id: controllerComboBox
                    portName: (model && model.track !== undefined) ? editorService.instrumentPortName(Number(model.track)) : ""
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    Layout.preferredWidth: 300
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The MIDI CC controller to automate")
                    onControllerChanged: function (newController) {
                        midiCcAutomationsModel.changeController(index, newController);
                    }
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
                    ToolTip.text: qsTr("The line where the interpolation starts")
                    onValueModified: model.line0 = value
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
                    ToolTip.text: qsTr("The line where the interpolation ends")
                    onValueModified: model.line1 = value
                }
                Label {
                    text: qsTr("Start value")
                    Layout.row: 0
                    Layout.column: 4
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startValueSpinBox
                    from: propertyService.minValue(model.controller)
                    to: propertyService.maxValue(model.controller)
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The controller value at the start line")
                    onValueModified: model.value0 = value
                }
                Label {
                    text: qsTr("End value")
                    Layout.row: 0
                    Layout.column: 5
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endValueSpinBox
                    from: propertyService.minValue(model.controller)
                    to: propertyService.maxValue(model.controller)
                    editable: true
                    enabled: model.line0 !== model.line1
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 5
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The controller value at the end line")
                    onValueModified: model.value1 = value
                }
            }
        }
        GroupBox {
            title: qsTr("Output")
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 4
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                Label {
                    text: qsTr("Events per beat")
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: eventsPerBeatSpinBox
                    from: 1
                    to: midiCcAutomationsModel.linesPerBeat
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Number of events to send per beat")
                    onValueModified: model.eventsPerBeat = value
                    onToChanged: value = Math.min(value, to)
                }
                Label {
                    text: qsTr("Line offset")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: lineOffsetSpinBox
                    from: 0
                    to: midiCcAutomationsModel.linesPerBeat - 1
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Line offset within the beat")
                    onValueModified: model.lineOffset = value
                    onToChanged: value = Math.min(value, to)
                }
            }
        }
        GroupBox {
            title: qsTr("Modulation")
            Layout.row: 1
            Layout.column: 5
            Layout.columnSpan: 4
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 5
                Label {
                    text: qsTr("Type")
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: modulationTypeComboBox
                    model: [qsTr("Sine Wave"), qsTr("Random")]
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The type of modulation to apply (Sine Wave or Random)")
                    onActivated: function (comboBoxIndex) {
                        midiCcAutomationsModel.changeModulationType(index, comboBoxIndex);
                    }
                }
                Label {
                    text: qsTr("Cycles")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationCyclesSpinBox
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
                    ToolTip.text: qsTr("The number of modulation cycles over the automation range")
                    onValueModified: model.modulationCycles = value
                }
                Label {
                    text: qsTr("Amplitude (%)")
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationAmplitudeSpinBox
                    from: 0
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The strength of the modulation as a percentage of the total controller range")
                    onValueModified: model.modulationAmplitude = value
                }
                Label {
                    text: qsTr("Offset (%)")
                    Layout.row: 0
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationOffsetSpinBox
                    from: -100
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("An additive constant value applied to the modulation")
                    onValueModified: model.modulationOffset = value
                }
                CheckBox {
                    id: modulationInvertedCheckBox
                    text: qsTr("Inverted")
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Invert the phase of the modulation")
                    onCheckedChanged: model.modulationInverted = checked
                    Component.onCompleted: checked = model.modulationInverted
                }
            }
        }
        Button {
            id: deleteButton
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.column: 9
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete this automation")
            Image {
                id: backgroundImage
                source: "../Graphics/delete.png"
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
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Optional comment for this automation")
            onTextChanged: model.comment = text
            Component.onCompleted: text = model.comment
        }
    }
    // Ensure initial value is set on component creation
    Component.onCompleted: initialize()
}
