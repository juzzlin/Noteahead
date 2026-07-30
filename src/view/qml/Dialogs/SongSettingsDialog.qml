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

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Song settings") + "</strong>"
    modal: true
    readonly property string _tag: "SongSettingsDialog"
    // An explicit button box rather than standardButtons, which stretches a lone button over the
    // whole dialog width.
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                rootItem.accepted();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Close this dialog")
        }
    }
    function initialize(): void {
        uiLogger.info(_tag, "Initializing");
        syncCheckbox.checked = songSettingsModel.autoNoteOffSyncEnabled;
        offsetSpinBox.value = songSettingsModel.autoNoteOffOffset;
        divisionComboBox.currentIndex = divisionComboBox.indexOfDenominator(songSettingsModel.autoNoteOffSyncDenominator);
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        GroupBox {
            title: qsTr("Timing")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                CheckBox {
                    id: syncCheckbox
                    text: qsTr("Sync auto note-off offset to BPM")
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Express the offset as a note length instead of milliseconds, so that it follows the tempo")
                    onCheckedChanged: songSettingsModel.autoNoteOffSyncEnabled = checked
                }
                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    Label {
                        text: syncCheckbox.checked ? qsTr("Auto note-off offset:") : qsTr("Auto note-off offset (ms):")
                    }
                    StackLayout {
                        Layout.fillWidth: true
                        currentIndex: syncCheckbox.checked ? 1 : 0
                        SpinBox {
                            id: offsetSpinBox
                            from: 0
                            to: 500
                            stepSize: 5
                            editable: true
                            Layout.fillWidth: true
                            Keys.onReturnPressed: focus = false
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Set offset for auto note-off events in milliseconds. This defines the time between a note-off and the following note-on in the same column.")
                            onValueModified: songSettingsModel.autoNoteOffOffset = value
                        }
                        ComboBox {
                            id: divisionComboBox
                            Layout.fillWidth: true
                            model: songSettingsModel.autoNoteOffSyncDenominators.map(denominator => `1/${denominator}`)
                            function indexOfDenominator(denominator: int): int {
                                const index = songSettingsModel.autoNoteOffSyncDenominators.indexOf(denominator);
                                return index >= 0 ? index : 0;
                            }
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Set offset for auto note-off events as a note length. This defines the time between a note-off and the following note-on in the same column.")
                            onActivated: songSettingsModel.autoNoteOffSyncDenominator = songSettingsModel.autoNoteOffSyncDenominators[currentIndex]
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: themeService.accentColor
                    text: qsTr("Applies to the whole song. Individual channels can override this in their timing settings.")
                }
            }
        }
        // Keeps the group boxes at the top instead of stretching them over the dialog's height.
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
