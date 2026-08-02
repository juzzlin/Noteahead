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

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings") + "</strong>"
    modal: true
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
            ToolTip.text: qsTr("Save current settings")
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10
        StackLayout {
            id: mainLayout
            height: parent.height - tabBar.height
            width: parent.width
            currentIndex: tabBar.currentIndex
            SettingsDialog_GeneralSettings {
                height: parent.height
                width: parent.width
            }
            SettingsDialog_MidiSettings {
                height: parent.height
                width: parent.width
            }
            SettingsDialog_AudioSettings {
                height: parent.height
                width: parent.width
            }
            SettingsDialog_ThemeSettings {
                height: parent.height
                width: parent.width
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("General")
            }
            TabButton {
                text: qsTr("MIDI")
            }
            TabButton {
                text: qsTr("Audio")
            }
            TabButton {
                text: qsTr("Theme")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
