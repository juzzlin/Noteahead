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
import "../Components"
import "ParameterAutomationTargets.js" as Targets

//! One parameter automation in the edit dialog. What it drives is shown but not edited: a different
//! target is a different automation.
GroupBox {
    id: delegateRoot
    title: qsTr("Pattern: %1, Track: %2, Column: %3").arg(model.pattern).arg(model.track).arg(model.column)

    property var targets: []
    readonly property var target: Targets.findTarget(targets, model.effectId)
    readonly property var parameter: Targets.findParameter(target, model.parameter)
    readonly property bool isRelative: model.mode === 1
    readonly property var knobSetup: Targets.knobSetup(parameter, isRelative)
    //! A ComboBox shadows the delegate's model with its own, so everything the combo boxes below read
    //! or write goes through the row as it is seen from here.
    readonly property var rowModel: model
    readonly property bool hasLineRange: model.line0 !== model.line1
    readonly property int curve: model.curve
    readonly property int modulationType: model.modulationType
    function setCurve(curve: int): void {
        rowModel.curve = curve;
    }
    function setMode(mode: int): void {
        rowModel.mode = mode;
    }
    function setModulationType(type: int): void {
        rowModel.modulationType = type;
    }

    GridLayout {
        anchors.fill: parent
        columns: 4
        rowSpacing: 10
        CheckBox {
            text: qsTr("Enabled")
            checked: model.enabled
            onToggled: model.enabled = checked
            Layout.column: 0
            Layout.row: 0
        }
        Label {
            // A target that is gone renders nothing, and says so here rather than silently
            text: delegateRoot.target ? Targets.effectLabel(delegateRoot.target) + " - " + (delegateRoot.parameter ? delegateRoot.parameter.label : model.parameter) : qsTr("Missing effect: this automation does nothing")
            color: delegateRoot.target ? themeService.accentColor : "orange"
            elide: Text.ElideMiddle
            Layout.column: 1
            Layout.row: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }
        GroupBox {
            title: qsTr("Interpolation")
            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 4
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 5
                Label {
                    text: qsTr("Start line")
                }
                Label {
                    text: qsTr("End line")
                }
                Label {
                    text: qsTr("Curve")
                }
                Label {
                    text: qsTr("Mode")
                }
                Item {
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 999
                    editable: true
                    value: model.line0
                    Keys.onReturnPressed: focus = false
                    onValueModified: model.line0 = value
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 999
                    editable: true
                    value: model.line1
                    Keys.onReturnPressed: focus = false
                    onValueModified: model.line1 = value
                    Layout.fillWidth: true
                }
                InterpolationCurveComboBox {
                    enabled: delegateRoot.hasLineRange
                    currentIndex: delegateRoot.curve
                    onActivated: comboBoxIndex => delegateRoot.setCurve(comboBoxIndex)
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: [qsTr("Absolute"), qsTr("Relative")]
                    currentIndex: delegateRoot.isRelative ? 1 : 0
                    onActivated: comboBoxIndex => delegateRoot.setMode(comboBoxIndex)
                    Layout.fillWidth: true
                }
                Item {
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Start value")
                    suffix: delegateRoot.knobSetup.suffix
                    mapping: delegateRoot.knobSetup.mapping
                    mapMin: delegateRoot.knobSetup.min
                    mapMax: delegateRoot.knobSetup.max
                    value: model.value0 * Constants.uiInternalScaling
                    onMoved: v => model.value0 = v / Constants.uiInternalScaling
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("End value")
                    enabled: delegateRoot.hasLineRange
                    suffix: delegateRoot.knobSetup.suffix
                    mapping: delegateRoot.knobSetup.mapping
                    mapMin: delegateRoot.knobSetup.min
                    mapMax: delegateRoot.knobSetup.max
                    value: model.value1 * Constants.uiInternalScaling
                    onMoved: v => model.value1 = v / Constants.uiInternalScaling
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                }
                Label {
                    text: delegateRoot.isRelative && delegateRoot.parameter ? qsTr("At the current setting: %1 to %2").arg(Targets.relativeReadout(delegateRoot.parameter, model.value0)).arg(Targets.relativeReadout(delegateRoot.parameter, model.value1)) : ""
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
        GroupBox {
            title: qsTr("Modulation")
            Layout.row: 2
            Layout.column: 0
            Layout.columnSpan: 4
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 5
                Label {
                    text: qsTr("Type")
                }
                Label {
                    text: qsTr("Cycles")
                }
                Label {
                    text: qsTr("Amplitude (%)")
                }
                Label {
                    text: qsTr("Offset (%)")
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: [qsTr("Sine Wave"), qsTr("Random")]
                    currentIndex: delegateRoot.modulationType
                    onActivated: comboBoxIndex => delegateRoot.setModulationType(comboBoxIndex)
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 127
                    editable: true
                    value: model.modulationCycles
                    Keys.onReturnPressed: focus = false
                    onValueModified: model.modulationCycles = value
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 200
                    editable: true
                    value: model.modulationAmplitude
                    Keys.onReturnPressed: focus = false
                    onValueModified: model.modulationAmplitude = value
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: -100
                    to: 100
                    editable: true
                    value: model.modulationOffset
                    Keys.onReturnPressed: focus = false
                    onValueModified: model.modulationOffset = value
                    Layout.fillWidth: true
                }
                CheckBox {
                    text: qsTr("Inverted")
                    checked: model.modulationInverted
                    onToggled: model.modulationInverted = checked
                }
            }
        }
        TextField {
            placeholderText: qsTr("Comment")
            text: model.comment
            onEditingFinished: model.comment = text
            Keys.onReturnPressed: focus = false
            Layout.row: 3
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
        }
        AppButton {
            Layout.row: 3
            Layout.column: 3
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete this automation")
            Image {
                source: "../Graphics/delete.png"
                sourceSize: Qt.size(parent.height, parent.height)
                height: parent.height
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
            }
            onClicked: parameterAutomationsModel.removeAt(index)
        }
    }
}
