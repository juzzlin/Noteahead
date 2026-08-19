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

AnimatedDialog {
    id: root

    property string deviceName: ""
    property int slotIndex: -1

    title: "<strong>" + qsTr("Device Settings: ") + deviceName + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.defaultDialogScale : 600
    height: parent ? parent.height * Constants.defaultDialogScale : 460

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onOpened: {
        root.slotIndex = deviceRackController.slotOfDevice(root.deviceName);
        faderPositionCombo.currentIndex = deviceRackController.deviceFaderPosition(root.slotIndex);
        sendTapCombo.currentIndex = deviceRackController.deviceSendTap(root.slotIndex);
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: 14

            Label {
                text: qsTr("Signal Flow")
                font.bold: true
                font.pixelSize: 16
                color: themeService.accentColor
            }

            Label {
                text: qsTr("Gain trims the device into its insert effects. Where the Fader sits decides whether it balances the device before or after them.")
                font.pixelSize: 12
                opacity: 0.7
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("Fader position:")
                    Layout.preferredWidth: 140
                }

                ComboBox {
                    id: faderPositionCombo
                    model: [qsTr("Before insert effects"), qsTr("After insert effects")]
                    Layout.fillWidth: true
                    onActivated: deviceRackController.setDeviceFaderPosition(root.slotIndex, currentIndex)
                }
            }

            Label {
                text: faderPositionCombo.currentIndex === 0 ? qsTr("The fader feeds the insert effects, so moving it changes how hard a compressor or saturator is driven. This is how devices behaved before this setting existed, and is kept as the default so older projects sound unchanged.") : qsTr("The insert effects run at the level Gain sets, and the fader only balances what comes out of them. Gain stage the device first, then balance without disturbing its dynamics.")
                font.pixelSize: 12
                opacity: 0.7
                Layout.fillWidth: true
                Layout.leftMargin: 150
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Effect Sends")
                font.bold: true
                font.pixelSize: 16
                color: themeService.accentColor
                Layout.topMargin: 10
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("Send tap:")
                    Layout.preferredWidth: 140
                }

                ComboBox {
                    id: sendTapCombo
                    model: [qsTr("Post-fader"), qsTr("Pre-fader")]
                    Layout.fillWidth: true
                    onActivated: deviceRackController.setDeviceSendTap(root.slotIndex, currentIndex)
                }
            }

            Label {
                text: sendTapCombo.currentIndex === 0 ? qsTr("Sends follow the fader, so pulling the device down takes its reverb with it and the wet/dry ratio stays put.") : qsTr("Sends keep their level whatever the fader does, so the device gets wetter as you pull it down.")
                font.pixelSize: 12
                opacity: 0.7
                Layout.fillWidth: true
                Layout.leftMargin: 150
                wrapMode: Text.WordWrap
            }
        }
    }
}
