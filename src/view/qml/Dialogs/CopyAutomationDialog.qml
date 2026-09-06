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

    //! Which kind of automation to list. The two are separate models with their own parameters, and
    //! the form that opened this one already knows which it is.
    property bool isPitchBend: false

    // A pure binding rather than a qsTr() down in the delegate's lookup below, which is a JS block
    // and so would keep whatever it was first evaluated in when the language changes.
    readonly property string pitchBendLabel: qsTr("Pitch Bend")

    //! The parameters of the picked automation, for the form to apply. Its location is left out:
    //! the new automation belongs where the form was opened.
    signal automationSelected(var values)

    function setTitle(text: string): void {
        title = `<strong>${text}</strong>`;
    }

    onOpened: {
        if (isPitchBend) {
            pitchBendAutomationsModel.requestPitchBendAutomations();
        } else {
            midiCcAutomationsModel.requestMidiCcAutomations();
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
            color: "#aaa"
            visible: !automationList.count
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: automationList
            model: rootItem.isPitchBend ? pitchBendAutomationsModel : midiCcAutomationsModel
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
                color: automationList.hoveredIndex === index ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"

                // Value ranges and controller names are per port, so the name has to be looked up
                // through the automation's own track rather than the one the form sits on.
                readonly property string portName: editorService.instrumentPortName(model.track)
                readonly property string controllerName: rootItem.isPitchBend ? rootItem.pitchBendLabel : row.midiCcName
                readonly property string midiCcName: {
                    if (rootItem.isPitchBend) {
                        return "";
                    }
                    const controllers = propertyService.getAvailableMidiControllers(row.portName);
                    for (let i = 0; i < controllers.length; i++) {
                        if (controllers[i].number === model.controller) {
                            return controllers[i].name;
                        }
                    }
                    return String(model.controller);
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
                            text: qsTr("Pattern %1, track %2 (%3), column %4").arg(model.pattern).arg(model.track).arg(editorService.trackName(model.track)).arg(model.column)
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
                            text: qsTr("Lines %1-%2, values %3-%4, %5").arg(model.line0).arg(model.line1).arg(model.value0).arg(model.value1).arg(curveNames.model[model.curve])
                            color: row.textColor
                            font.pointSize: 9
                        }
                        Text {
                            text: model.comment
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
                        // Built here rather than read back through the model afterwards: the roles
                        // are only in scope inside the delegate.
                        rootItem.automationSelected({
                            "controller": rootItem.isPitchBend ? 0 : model.controller,
                            "line0": model.line0,
                            "line1": model.line1,
                            "value0": model.value0,
                            "value1": model.value1,
                            "curve": model.curve,
                            "eventsPerBeat": rootItem.isPitchBend ? 0 : model.eventsPerBeat,
                            "lineOffset": rootItem.isPitchBend ? 0 : model.lineOffset,
                            "modulationType": model.modulationType,
                            "modulationCycles": model.modulationCycles,
                            "modulationAmplitude": model.modulationAmplitude,
                            "modulationOffset": model.modulationOffset,
                            "modulationInverted": model.modulationInverted,
                            "comment": model.comment
                        });
                        rootItem.close();
                    }
                }
            }
        }
    }
}
