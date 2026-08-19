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
    title: "<strong>" + qsTr("Track settings for track %1: '%2'").arg(trackSettingsModel.trackIndex + 1).arg(editorService.trackName(trackSettingsModel.trackIndex)) + "</strong>"
    modal: true
    property bool _initializing: false
    readonly property string _tag: "TrackSettingsDialog"
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function setPortName(name) {
        instrumentSettings.setPortName(name);
    }
    function initialize() {
        uiLogger.info(_tag, "Initializing");
        instrumentSettings.initialize();
        timingSettings.initialize();
        midiEffects.initialize();
        tabBar.currentIndex = 0;
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
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
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save current settings")
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Don't save current settings")
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
                TrackSettingsDialog_InstrumentSettings {
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
                TrackSettingsDialog_TimingSettings {
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
                TrackSettingsDialog_MidiEffects {
                    id: midiEffects
                    width: midiEffectsScrollView.availableWidth
                }
            }
            TrackSettingsDialog_MidiCcSettings {
                id: midiCcSettings
                Layout.fillWidth: true
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
            TabButton {
                text: qsTr("MIDI CC Settings")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
