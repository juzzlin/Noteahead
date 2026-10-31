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
import Noteahead 1.0
import "../Components"

Item {
    id: rootItem
    function startValue() {
        return startValueSpinBox.value;
    }
    function setStartValue(value) {
        startValueSpinBox.value = value;
    }
    function endValue(value) {
        return endValueSpinBox.value;
    }
    function setEndValue(value) {
        endValueSpinBox.value = value;
    }
    function startLine() {
        return startLineSpinBox.value;
    }
    function setStartLine(value) {
        startLineSpinBox.value = value;
    }
    function endLine() {
        return endLineSpinBox.value;
    }
    function setEndLine(value) {
        endLineSpinBox.value = value;
    }
    function comment() {
        return commentEdit.text;
    }
    function setComment(comment) {
        commentEdit.text = comment;
    }
    function curve(): int {
        return curveComboBox.currentIndex;
    }
    function setCurve(curve: int): void {
        curveComboBox.currentIndex = curve;
    }
    //! 0 for every tick, which is what makes a bend smooth; -1 for once a line, as automations made
    //! before the setting play; otherwise that many a beat, on the lines.
    function eventsPerBeat(): int {
        if (smoothCheckBox.checked) {
            return 0;
        }
        return eventsPerBeatSpinBox.value === eventsPerBeatSpinBox.to ? -1 : eventsPerBeatSpinBox.value;
    }
    function setEventsPerBeat(value: int): void {
        smoothCheckBox.checked = value === 0;
        eventsPerBeatSpinBox.value = value > 0 ? value : eventsPerBeatSpinBox.to;
    }
    function lineOffset(): int {
        return eventsPerBeat() > 0 ? lineOffsetSpinBox.value : 0;
    }
    function setLineOffset(value: int): void {
        lineOffsetSpinBox.value = value;
    }

    function resetModulations(): void {
        setModulationType(0);
        setCycles(0);
        setAmplitude(0);
        setOffset(0);
        setInverted(false);
    }
    function modulationType(): int {
        return modulationGroupBox.modulationType;
    }
    function setModulationType(type: int): void {
        modulationGroupBox.modulationType = type;
    }
    function cycles(): int {
        return modulationGroupBox.cycles;
    }
    function setCycles(cycles: int): void {
        modulationGroupBox.cycles = cycles;
    }
    function amplitude(): int {
        return modulationGroupBox.amplitude;
    }
    function setAmplitude(amplitude: int): void {
        modulationGroupBox.amplitude = amplitude;
    }
    function offset(): int {
        return modulationGroupBox.offset;
    }
    function setOffset(value: int): void {
        modulationGroupBox.offset = value;
    }
    function inverted(): bool {
        return modulationGroupBox.inverted;
    }
    function setInverted(inverted: bool): void {
        modulationGroupBox.inverted = inverted;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        SectionBox {
            title: qsTr("Interpolation settings")
            Layout.fillWidth: true
            GridLayout {
                rowSpacing: 10
                width: parent.width
                Label {
                    text: qsTr("Start line")
                    Layout.row: 0
                    Layout.column: 0
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("End line")
                    Layout.row: 0
                    Layout.column: 1
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Start value (%)")
                    Layout.row: 0
                    Layout.column: 2
                }
                SpinBox {
                    id: startValueSpinBox
                    from: -100
                    to: 100
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                Label {
                    text: " " + qsTr("End value (%)")
                    Layout.row: 0
                    Layout.column: 3
                }
                SpinBox {
                    id: endValueSpinBox
                    from: -100
                    to: 100
                    value: 0
                    editable: true
                    enabled: startLine() !== endLine()
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                Label {
                    text: " " + qsTr("Curve")
                    Layout.row: 0
                    Layout.column: 4
                }
                InterpolationCurveComboBox {
                    id: curveComboBox
                    enabled: startLine() !== endLine()
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                }
            }
        }
        SectionBox {
            title: qsTr("Output settings")
            Layout.fillWidth: true
            GridLayout {
                width: parent.width
                columns: 3
                rowSpacing: 10
                CheckBox {
                    id: smoothCheckBox
                    text: qsTr("Smooth")
                    checked: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Move the bend on every tick. Turn off to step it on the lines instead.")
                }
                Label {
                    text: qsTr("Events per beat")
                    enabled: !smoothCheckBox.checked
                }
                Label {
                    text: qsTr("Line offset")
                    enabled: !smoothCheckBox.checked
                }
                Item {
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: eventsPerBeatSpinBox
                    from: 1
                    to: editorService.linesPerBeat
                    value: editorService.linesPerBeat
                    enabled: !smoothCheckBox.checked
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: lineOffsetSpinBox
                    from: 0
                    to: editorService.linesPerBeat - 1
                    enabled: !smoothCheckBox.checked && eventsPerBeatSpinBox.value < eventsPerBeatSpinBox.to
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.fillWidth: true
                }
            }
        }
        ModulationGroupBox {
            id: modulationGroupBox
        }
        SectionBox {
            title: qsTr("Comment")
            Layout.fillWidth: true
            Layout.row: 2
            Layout.column: 0
            TextField {
                id: commentEdit
                readOnly: false
                width: parent.width
                Keys.onReturnPressed: {
                    focus = false;
                }
            }
        }
    }
}
