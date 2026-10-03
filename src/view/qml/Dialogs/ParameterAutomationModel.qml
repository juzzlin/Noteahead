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
import "ParameterAutomationTargets.js" as Targets

//! The form of a parameter automation: which effect and parameter it drives, how, and over which
//! lines. Values are knob positions, 0 to 1, and are shown and typed in the parameter's own units.
Item {
    id: rootItem

    implicitHeight: column.implicitHeight

    //! Read when the form is opened rather than bound: the list is built by walking every rack.
    property var targets: []
    readonly property var currentTarget: targetComboBox.currentIndex >= 0 && targetComboBox.currentIndex < targets.length ? targets[targetComboBox.currentIndex] : null
    readonly property var currentParameter: currentTarget && parameterComboBox.currentIndex >= 0 && parameterComboBox.currentIndex < currentTarget.parameters.length ? currentTarget.parameters[parameterComboBox.currentIndex] : null
    readonly property bool isRelative: modeComboBox.currentIndex === 1
    readonly property var knobSetup: Targets.knobSetup(currentParameter, isRelative)
    property real startPosition: 0
    property real endPosition: 1

    function refreshTargets(): void {
        targets = propertyService.automationTargets();
        targetComboBox.currentIndex = targets.length ? 0 : -1;
        parameterComboBox.currentIndex = 0;
    }
    function hasTarget(): bool {
        return currentTarget !== null && currentParameter !== null;
    }
    function effectId(): string {
        return currentTarget ? currentTarget.effectId : "";
    }
    function parameterKey(): string {
        return currentParameter ? currentParameter.key : "";
    }
    function mode(): int {
        return modeComboBox.currentIndex;
    }
    function startValue(): real {
        return startPosition;
    }
    function endValue(): real {
        return endPosition;
    }
    function startLine(): int {
        return startLineSpinBox.value;
    }
    function setStartLine(value: int): void {
        startLineSpinBox.value = value;
    }
    function endLine(): int {
        return endLineSpinBox.value;
    }
    function setEndLine(value: int): void {
        endLineSpinBox.value = value;
    }
    function curve(): int {
        return curveComboBox.currentIndex;
    }
    //! 0 for every tick, which is what makes a sweep smooth; otherwise that many a beat, on the lines.
    function eventsPerBeat(): int {
        return smoothCheckBox.checked ? 0 : eventsPerBeatSpinBox.value;
    }
    function lineOffset(): int {
        return smoothCheckBox.checked ? 0 : lineOffsetSpinBox.value;
    }
    function modulationType(): int {
        return modulationGroupBox.modulationType;
    }
    function cycles(): int {
        return modulationGroupBox.cycles;
    }
    function amplitude(): int {
        return modulationGroupBox.amplitude;
    }
    function offset(): int {
        return modulationGroupBox.offset;
    }
    function inverted(): bool {
        return modulationGroupBox.inverted;
    }
    function comment(): string {
        return commentEdit.text;
    }
    function reset(): void {
        modeComboBox.currentIndex = 0;
        startPosition = 0;
        endPosition = 1;
        curveComboBox.currentIndex = 0;
        smoothCheckBox.checked = true;
        lineOffsetSpinBox.value = 0;
        modulationGroupBox.modulationType = 0;
        modulationGroupBox.cycles = 0;
        modulationGroupBox.amplitude = 0;
        modulationGroupBox.offset = 0;
        modulationGroupBox.inverted = false;
        commentEdit.text = "";
    }

    ColumnLayout {
        id: column
        width: rootItem.width
        spacing: 10
        Label {
            text: qsTr("There are no effects to automate. Add one to a rack first.")
            visible: !rootItem.targets.length
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        GroupBox {
            title: qsTr("Target")
            enabled: rootItem.targets.length > 0
            Layout.fillWidth: true
            GridLayout {
                width: parent.width
                columns: 3
                rowSpacing: 10
                Label {
                    text: qsTr("Effect")
                }
                Label {
                    text: qsTr("Parameter")
                }
                Label {
                    text: qsTr("Mode")
                }
                ComboBox {
                    id: targetComboBox
                    model: rootItem.targets.map(target => Targets.effectLabel(target))
                    onActivated: parameterComboBox.currentIndex = 0
                    Layout.fillWidth: true
                    Layout.preferredWidth: 3
                }
                ComboBox {
                    id: parameterComboBox
                    model: rootItem.currentTarget ? rootItem.currentTarget.parameters.map(parameter => parameter.label) : []
                    Layout.fillWidth: true
                    Layout.preferredWidth: 2
                }
                ComboBox {
                    id: modeComboBox
                    model: [qsTr("Absolute"), qsTr("Relative")]
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Absolute sets the value itself. Relative goes from the control's rest at 0% to the knob's own setting at 100%, like Expression.")
                }
            }
        }
        GroupBox {
            title: qsTr("Interpolation settings")
            Layout.fillWidth: true
            GridLayout {
                width: parent.width
                columns: 3
                rowSpacing: 10
                Label {
                    text: qsTr("Start line")
                }
                Label {
                    text: qsTr("End line")
                }
                Label {
                    text: qsTr("Curve")
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.fillWidth: true
                }
                InterpolationCurveComboBox {
                    id: curveComboBox
                    enabled: rootItem.startLine() !== rootItem.endLine()
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Start value")
                    suffix: rootItem.knobSetup.suffix
                    mapping: rootItem.knobSetup.mapping
                    mapMin: rootItem.knobSetup.min
                    mapMax: rootItem.knobSetup.max
                    value: rootItem.startPosition * Constants.uiInternalScaling
                    onMoved: v => rootItem.startPosition = v / Constants.uiInternalScaling
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("End value")
                    enabled: rootItem.startLine() !== rootItem.endLine()
                    suffix: rootItem.knobSetup.suffix
                    mapping: rootItem.knobSetup.mapping
                    mapMin: rootItem.knobSetup.min
                    mapMax: rootItem.knobSetup.max
                    value: rootItem.endPosition * Constants.uiInternalScaling
                    onMoved: v => rootItem.endPosition = v / Constants.uiInternalScaling
                    Layout.fillWidth: true
                }
                Label {
                    // What the percentages come to with the knob where it is now
                    text: rootItem.isRelative && rootItem.currentParameter ? qsTr("At the current setting: %1 to %2").arg(Targets.relativeReadout(rootItem.currentParameter, rootItem.startPosition)).arg(Targets.relativeReadout(rootItem.currentParameter, rootItem.endPosition)) : ""
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
        GroupBox {
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
                    ToolTip.text: qsTr("Move the value on every tick. Turn off to step it on the lines instead.")
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
                    enabled: !smoothCheckBox.checked
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.fillWidth: true
                }
            }
        }
        ModulationGroupBox {
            id: modulationGroupBox
        }
        GroupBox {
            title: qsTr("Comment")
            Layout.fillWidth: true
            TextField {
                id: commentEdit
                width: parent.width
                Keys.onReturnPressed: focus = false
            }
        }
    }
}
