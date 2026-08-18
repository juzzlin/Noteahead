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

GroupBox {
    width: parent.width
    title: qsTr("MIDI")
    ColumnLayout {
        width: parent.width
        spacing: 10
        GroupBox {
            title: qsTr("Controller")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 3
                width: parent.width
                Label {
                    text: qsTr("Port:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 0
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: portNameDropdown
                    model: midiSettingsModel.midiInPorts
                    currentIndex: 0
                    Layout.column: 2
                    Layout.columnSpan: 7
                    Layout.row: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set the port of a MIDI controller keyboard")
                    function updateSelection() {
                        const index = find(midiSettingsModel.controllerPort);
                        if (index !== -1) {
                            currentIndex = index;
                        }
                    }
                    onCurrentTextChanged: midiSettingsModel.controllerPort = currentText
                    Connections {
                        target: midiSettingsModel
                        function onMidiInPortsChanged() {
                            portNameDropdown.updateSelection();
                        }
                    }
                    Component.onCompleted: updateSelection()
                }
                Label {
                    text: qsTr("Data:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 1
                    Layout.fillWidth: true
                }
                TextField {
                    text: midiSettingsModel.debugData
                    readOnly: true
                    Layout.column: 2
                    Layout.columnSpan: 7
                    Layout.row: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Received MIDI data")
                }
            }
        }
        GroupBox {
            title: qsTr("Synchronization")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 3
                width: parent.width
                RowLayout {
                    Layout.column: 0
                    Layout.columnSpan: 9
                    Layout.row: 0
                    Layout.fillWidth: true
                    spacing: 20
                    CheckBox {
                        text: qsTr("Jack transport sync")
                        enabled: settingsService.audioBackend === 3
                        checked: midiSettingsModel.jackSyncEnabled
                        onCheckedChanged: midiSettingsModel.jackSyncEnabled = checked
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: settingsService.audioBackend === 3 ? qsTr("Sync Noteahead playback with Jack transport") : qsTr("Jack transport sync requires JACK audio backend")
                    }
                    CheckBox {
                        text: qsTr("Jack BPM sync")
                        enabled: midiSettingsModel.jackSyncEnabled && settingsService.audioBackend === 3
                        checked: midiSettingsModel.jackBpmSyncEnabled
                        onCheckedChanged: midiSettingsModel.jackBpmSyncEnabled = checked
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: settingsService.audioBackend === 3 ? qsTr("Sync Noteahead BPM with Jack") : qsTr("Jack BPM sync requires JACK audio backend")
                    }
                }
                CheckBox {
                    text: qsTr("MIDI transport sync")
                    Layout.column: 0
                    Layout.columnSpan: 9
                    Layout.row: 1
                    Layout.fillWidth: true
                    checked: midiSettingsModel.midiSyncEnabled
                    onCheckedChanged: midiSettingsModel.midiSyncEnabled = checked
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Sync Noteahead playback with MIDI transport")
                }
                Label {
                    Layout.column: 0
                    Layout.columnSpan: 9
                    Layout.row: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: themeService.accentColor
                    text: qsTr("Jack mode enabled (input_1/input_2). Use a patchbay like Carla to route audio to Noteahead.")
                    visible: settingsService.audioBackend === 3
                }
            }
        }
        GroupBox {
            title: qsTr("Miscellaneous")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 2
                width: parent.width
                Label {
                    text: qsTr("Default auto note-off offset (ms):")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: autoNoteOffOffsetSpinBox
                    from: 0
                    to: 500
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 0
                    Layout.fillWidth: true
                    value: settingsService.autoNoteOffOffset()
                    editable: true
                    onValueChanged: settingsService.setAutoNoteOffOffset(value)
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set the offset new songs start with, in milliseconds. This defines the time between a note-off and the following note-on in the same column.")
                }
                Label {
                    Layout.column: 0
                    Layout.columnSpan: 9
                    Layout.row: 1
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: themeService.accentColor
                    text: qsTr("Applies to new songs. Override per song in Song → Settings, or per channel in Track Settings.")
                }
            }
        }
    }
}
