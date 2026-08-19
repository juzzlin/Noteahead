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
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Column settings for track %1, column %2").arg(columnSettingsModel.trackIndex + 1).arg(columnSettingsModel.columnIndex + 1) + "</strong>"
    modal: true
    function initialize() {
        instrumentSettings.initialize();
        timingSettings.initialize();
        midiEffects.initialize();
        tabBar.currentIndex = 0;
    }
    function saveSettings() {
        columnSettingsModel.save();
    }
    function setColumn(trackIndex, columnIndex) {
        columnSettingsModel.trackIndex = trackIndex;
        columnSettingsModel.columnIndex = columnIndex;
        columnSettingsModel.requestData();
    }
    // Cancel here throws away every edit made in the dialog, so the buttons take the whole
    // footer width instead of sitting side by side in the corner
    stretchFooterButtons: true

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
                rootItem.accepted();
            }
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10

        StackLayout {
            height: parent.height - tabBar.height - parent.spacing
            width: parent.width
            currentIndex: tabBar.currentIndex

            ScrollView {
                id: instrumentScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_InstrumentSettings {
                    id: instrumentSettings
                    width: instrumentScrollView.availableWidth
                }
            }

            ScrollView {
                id: timingScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_TimingSettings {
                    id: timingSettings
                    width: timingScrollView.availableWidth
                }
            }

            ScrollView {
                id: midiEffectsScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_MidiEffects {
                    id: midiEffects
                    width: midiEffectsScrollView.availableWidth
                }
            }
        }

        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("Instrument")
            }
            TabButton {
                text: qsTr("Timing")
            }
            TabButton {
                text: qsTr("MIDI Effects")
            }
        }
    }

    Component.onCompleted: {
        columnSettingsModel.dataReceived.connect(initialize);
    }
}
