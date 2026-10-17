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
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"
import "ParameterAutomationTargets.js" as Targets

// Picks an automation to start a new one from. Opened on top of the Add automation form, which is
// what it fills in: nothing is written here, so the copy can still be retuned or abandoned.
AnimatedDialog {
    id: rootItem
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600
    // Cancel rather than Ok: picking a row is what commits, so there is nothing for an accept button
    // to do that clicking the row has not already done.
    standardButtons: Dialog.Cancel

    //! Which kind of automation to list: "midiCc", "pitchBend" or "parameter". They carry different
    //! parameters, and whatever opened this dialog already knows which of them it is filling.
    property string kind: "midiCc"
    readonly property bool isPitchBend: kind === "pitchBend"
    readonly property bool isParameter: kind === "parameter"
    //! For naming what a parameter automation drives. Read when the dialog opens.
    property var targets: []

    // A pure binding rather than a qsTr() down in the delegate's lookup below, which is a JS block
    // and so would keep whatever it was first evaluated in when the language changes.
    readonly property string pitchBendLabel: qsTr("Pitch Bend")

    //! The parameters of the picked automation, for the caller to apply. It carries the location
    //! keys too, but nothing applies those: an automation stays where it already is.
    signal automationSelected(var values)

    function setTitle(text: string): void {
        title = `<strong>${text}</strong>`;
    }

    //! The song's automations, snapshotted when the dialog opens. A plain list rather than the edit
    //! dialogs' own model, which this one can be opened on top of: re-scoping that model would pull
    //! the list out from under the edit it was opened from.
    property var automations: []

    onOpened: {
        if (isParameter) {
            targets = propertyService.automationTargets();
            automations = automationService.parameterAutomationsAsVariantList();
        } else {
            automations = isPitchBend ? automationService.pitchBendAutomationsAsVariantList() : automationService.midiCcAutomationsAsVariantList();
        }
    }

    // The curve names live in InterpolationCurveComboBox and nowhere else. One hidden instance reads
    // them here rather than a second copy that could drift out of step with Interpolator::CurveType,
    // and being the same binding it retranslates with the rest of the UI.
    InterpolationCurveComboBox {
        id: curveNames
        visible: false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Label {
            text: qsTr("No automations to copy.")
            color: Theme.textSecondary
            visible: !automationList.count
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: automationList
            model: rootItem.automations
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AlwaysOff
            }
            delegate: Rectangle {
                id: row
                width: automationList.width
                height: rowContent.implicitHeight + 16
                color: automationList.hoveredIndex === index ? themeService.accentColor : Theme.surfacePanel
                radius: 5
                border.color: Theme.borderStrong

                // Value ranges and controller names are per port, so the name has to be looked up
                // through the automation's own track rather than the one the form sits on.
                readonly property string portName: editorService.instrumentPortName(modelData.track)
                readonly property string controllerName: rootItem.isPitchBend ? rootItem.pitchBendLabel : (rootItem.isParameter ? row.parameterName : row.midiCcName)
                readonly property var target: rootItem.isParameter ? Targets.findTarget(rootItem.targets, modelData.owner, modelData.ownerId, modelData.parameter) : null
                readonly property var parameter: Targets.findParameter(row.target, modelData.parameter)
                readonly property string parameterName: row.target ? Targets.targetLabel(row.target) + " - " + (row.parameter ? row.parameter.label : modelData.parameter) : Targets.targetLabel(null)
                //! Positions read out the way the knob reads them, or as percentages for a relative one
                function valueText(position: real): string {
                    if (!rootItem.isParameter) {
                        return String(position);
                    }
                    const setup = Targets.knobSetup(row.parameter, modelData.mode === 1);
                    return knobController.format(position, setup.mapping, setup.suffix, setup.min, setup.max);
                }
                readonly property string midiCcName: {
                    if (rootItem.isPitchBend || rootItem.isParameter) {
                        return "";
                    }
                    const controllers = propertyService.getAvailableMidiControllers(row.portName);
                    for (let i = 0; i < controllers.length; i++) {
                        if (controllers[i].number === modelData.controller) {
                            return controllers[i].name;
                        }
                    }
                    return String(modelData.controller);
                }
                readonly property color textColor: automationList.hoveredIndex === index ? themeService.accentTextColor : "white"

                ColumnLayout {
                    id: rowContent
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: qsTr("Pattern %1, track %2 (%3), column %4").arg(modelData.pattern).arg(modelData.track).arg(editorService.trackName(modelData.track)).arg(modelData.column)
                            color: row.textColor
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: row.controllerName
                            color: row.textColor
                            font.bold: true
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: qsTr("Lines %1-%2, values %3-%4, %5").arg(modelData.line0).arg(modelData.line1).arg(row.valueText(modelData.value0)).arg(row.valueText(modelData.value1)).arg(curveNames.model[modelData.curve])
                            color: row.textColor
                            font.pointSize: 9
                        }
                        Text {
                            text: modelData.comment
                            color: row.textColor
                            font.pointSize: 9
                            font.italic: true
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: automationList.hoveredIndex = index
                    onExited: automationList.hoveredIndex = -1
                    onClicked: {
                        automationList.hoveredIndex = -1;
                        // The snapshot entry is already the shape the forms apply, location keys and
                        // all; those are simply not read.
                        rootItem.automationSelected(modelData);
                        rootItem.close();
                    }
                }
            }
        }
    }
}
