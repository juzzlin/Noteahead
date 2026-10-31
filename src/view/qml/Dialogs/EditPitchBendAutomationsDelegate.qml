// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../ToolBar"
import "../Components"

SectionBox {
    id: delegateRoot
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`
    //! ComboBox shadows the delegate's model with its own, so the line range has to be resolved here
    readonly property bool hasLineRange: model.line0 !== model.line1
    //! The row the curve dialog is opened with.
    readonly property var rowModel: model
    // The title with the curve after it, so each row shows its shape without opening it
    label: AutomationRowHeading {
        x: delegateRoot.leftPadding
        width: delegateRoot.availableWidth
        text: delegateRoot.title
        shape: delegateRoot.rowModel.curveShape
        line0: delegateRoot.rowModel.line0
        line1: delegateRoot.rowModel.line1
        minValue: -100
        maxValue: 100
    }
    //! Draws this automation's curve, in the dialog that writes it back into this row.
    function openCurveDialog(): void {
        UiService.requestAutomationCurveDialog("pitchBend", index, {
            "drawnCurve": delegateRoot.rowModel.drawnCurve,
            "line0": delegateRoot.rowModel.line0,
            "line1": delegateRoot.rowModel.line1,
            "value0": delegateRoot.rowModel.value0,
            "value1": delegateRoot.rowModel.value1,
            "minValue": -100,
            "maxValue": 100,
            "valueStep": 0,
            "formatValue": value => value.toFixed(1) + " %",
            "title": qsTr("Draw Pitch Bend automation curve")
        });
    }

    // Copying replaces this automation wholesale, and the controls below are set once on creation
    // rather than bound to the model, so they have to be re-read or the row would go on showing what
    // it was replaced from. A dedicated signal rather than dataChanged: that fires on every keystroke
    // of an ordinary edit, and re-reading the controls under the one being typed into is not wanted.
    Connections {
        target: pitchBendAutomationsModel
        function onAutomationReplaced(replacedIndex: int): void {
            if (replacedIndex === index) {
                delegateRoot.initialize();
            }
        }
    }

    //! What the output controls stand for, as the model stores it: 0 for smooth, -1 for once a line.
    function outputEventsPerBeat(): int {
        if (smoothCheckBox.checked) {
            return 0;
        }
        return eventsPerBeatSpinBox.value === eventsPerBeatSpinBox.to ? -1 : eventsPerBeatSpinBox.value;
    }

    function initialize(): void {
        if (model) {
            startLineSpinBox.value = model.line0;
            endLineSpinBox.value = model.line1;
            startValueSpinBox.value = model.value0;
            endValueSpinBox.value = model.value1;
            curveComboBox.currentIndex = model.curve;
            smoothCheckBox.checked = model.eventsPerBeat === 0;
            eventsPerBeatSpinBox.value = model.eventsPerBeat > 0 ? model.eventsPerBeat : eventsPerBeatSpinBox.to;
            lineOffsetSpinBox.value = model.lineOffset;
            modulationTypeComboBox.currentIndex = model.modulationType;
            modulationCyclesSpinBox.value = model.modulationCycles;
            modulationAmplitudeSpinBox.value = model.modulationAmplitude;
            modulationOffsetSpinBox.value = model.modulationOffset;
            modulationInvertedCheckBox.checked = model.modulationInverted;
            commentEdit.text = model.comment;
            enableCheckbox.checked = model.enabled;
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
        SectionBox {
            title: qsTr("Interpolation")
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 5
                Label {
                    text: qsTr("Start line")
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 0
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
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The line where the interpolation ends")
                    onValueModified: model.line1 = value
                }
                Label {
                    text: qsTr("Start value (%)")
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startValueSpinBox
                    from: -100
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The pitch bend value at the start line (-100% to 100%)")
                    onValueModified: model.value0 = value
                }
                Label {
                    text: qsTr("End value (%)")
                    Layout.row: 0
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endValueSpinBox
                    from: -100
                    to: 100
                    editable: true
                    enabled: model.line0 !== model.line1
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The pitch bend value at the end line (-100% to 100%)")
                    onValueModified: model.value1 = value
                }
                Label {
                    text: qsTr("Curve")
                    Layout.row: 0
                    Layout.column: 4
                    Layout.fillWidth: true
                }
                InterpolationCurveComboBox {
                    id: curveComboBox
                    enabled: delegateRoot.hasLineRange
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                    drawnAvailable: true
                    // Drawn starts from the ramp it replaces, and goes straight on to drawing
                    onActivated: function (comboBoxIndex) {
                        pitchBendAutomationsModel.changeCurve(index, comboBoxIndex);
                        if (comboBoxIndex === drawnIndex) {
                            delegateRoot.openCurveDialog();
                        }
                    }
                }
            }
        }
        SectionBox {
            title: qsTr("Output")
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 4
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 3
                CheckBox {
                    id: smoothCheckBox
                    text: qsTr("Smooth")
                    Layout.row: 1
                    Layout.column: 0
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Move the bend on every tick. Turn off to step it on the lines instead.")
                    onToggled: model.eventsPerBeat = delegateRoot.outputEventsPerBeat()
                }
                Label {
                    text: qsTr("Events per beat")
                    enabled: !smoothCheckBox.checked
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: eventsPerBeatSpinBox
                    from: 1
                    to: editorService.linesPerBeat
                    enabled: !smoothCheckBox.checked
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Number of events to send per beat")
                    onValueModified: model.eventsPerBeat = delegateRoot.outputEventsPerBeat()
                    onToChanged: value = Math.min(value, to)
                }
                Label {
                    text: qsTr("Line offset")
                    enabled: lineOffsetSpinBox.enabled
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: lineOffsetSpinBox
                    from: 0
                    to: editorService.linesPerBeat - 1
                    enabled: !smoothCheckBox.checked && eventsPerBeatSpinBox.value < eventsPerBeatSpinBox.to
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
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
        SectionBox {
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
                        pitchBendAutomationsModel.changeModulationType(index, comboBoxIndex);
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
                    to: 200
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The strength of the modulation as a percentage of the pitch bend half-range")
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
        // Under the comment field and exactly as wide as it, the two sharing that width evenly. The
        // grid's own columns are sized by the groups above, so the even split comes from a row of its
        // own rather than from a column span.
        RowLayout {
            Layout.row: 3
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            spacing: 10
            AppButton {
                id: copyButton
                text: qsTr("Copy...")
                Layout.fillWidth: true
                // An even split has to be asked for: fillWidth shares out what is left over from each
                // item's own implicit width, and a labelled button starts wider than an icon.
                Layout.preferredWidth: 0
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Replace this automation with the settings of another one")
                onClicked: UiService.requestCopyAutomationDialog("pitchBend", index)
            }
            AppButton {
                text: qsTr("Draw curve...")
                enabled: delegateRoot.hasLineRange
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Draw the curve node by node")
                onClicked: delegateRoot.openCurveDialog()
            }
            AppButton {
                id: deleteButton
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Delete this automation")
                Image {
                    id: backgroundImage
                    source: "../Graphics/delete.png"
                    sourceSize: Qt.size(parent.height, parent.height)
                    height: parent.height
                    anchors.centerIn: parent
                    fillMode: Image.PreserveAspectFit
                }
                onClicked: pitchBendAutomationsModel.removeAt(index)
            }
        }
    }
    Component.onCompleted: initialize()
}
