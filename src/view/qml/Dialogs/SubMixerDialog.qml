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
    property int slotIndex: -1
    property var candidates: []

    title: "<strong>" + qsTr("Sub Mixer (Slot %1)").arg(slotIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 640

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    function reload(): void {
        root.candidates = slotIndex >= 0 ? deviceRackController.subMixerCandidates(slotIndex) : [];
    }

    onOpened: reload()

    // Wide like the device, effect and settings dialogs these open alongside
    stretchFooterButtons: true

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: qsTr("Members")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: memberListView
            model: root.candidates
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var modelData
                required property int index
                width: memberListView.width
                height: 60
                color: (memberListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: memberListView.hoveredIndex = index
                    onExited: memberListView.hoveredIndex = -1
                    onClicked: {
                        memberListView.hoveredIndex = -1;
                        deviceRackController.openDevice(modelData.slot);
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    CheckBox {
                        checked: modelData.member
                        // A device already claimed by another Sub Mixer, or one that would close a
                        // routing loop, is shown but not selectable rather than silently stolen.
                        enabled: modelData.member || !modelData.blocked
                        Layout.preferredWidth: 32
                        onToggled: {
                            if (checked) {
                                deviceRackController.addSubMixerMember(root.slotIndex, modelData.slot);
                            } else {
                                deviceRackController.removeSubMixerMember(root.slotIndex, modelData.slot);
                            }
                            root.reload();
                        }
                    }

                    Text {
                        text: qsTr("Slot %1: %2 (%3)").arg(modelData.slot + 1).arg(modelData.name).arg(modelData.typeName)
                        color: "white"
                        font.pointSize: 13
                        font.bold: memberListView.hoveredIndex === index && root.activeFocus
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: modelData.owner >= 0 && !modelData.member ? qsTr("in Sub Mixer %1").arg(modelData.owner + 1) : (modelData.blocked ? qsTr("would create a loop") : "")
                        color: "#aaa"
                        font.pointSize: 11
                        font.italic: true
                        horizontalAlignment: Text.AlignRight
                        visible: text !== ""
                    }

                    Text {
                        text: modelData.trackNames
                        color: "#aaa"
                        font.pointSize: 11
                        Layout.preferredWidth: 150
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                    }

                    AppButton {
                        text: qsTr("Sends")
                        onClicked: UiService.requestEffectSendsDialog(modelData.name)
                        Layout.preferredWidth: 80
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            // Each setter reaches revisionChanged via Device::dataChanged, so referencing revision
            // is what makes these bindings re-evaluate -- both after a drag and when MIDI CC moves
            // Volume or Pan underneath them. Without a tracked dependency a knob reads once and
            // snaps back to its stale value the moment the drag ends.
            Knob {
                label: qsTr("Gain")
                mapping: "decibel"
                mapMin: -30
                mapMax: 30
                Layout.fillWidth: true
                value: {
                    deviceRackController.revision;
                    return deviceRackController.deviceGain(root.slotIndex);
                }
                onMoved: v => deviceRackController.setDeviceGain(root.slotIndex, v)
            }

            Knob {
                label: qsTr("Fader")
                mapping: "fader"
                Layout.fillWidth: true
                value: {
                    deviceRackController.revision;
                    return deviceRackController.deviceVolume(root.slotIndex);
                }
                onMoved: v => deviceRackController.setDeviceVolume(root.slotIndex, v)
            }

            Knob {
                label: qsTr("Pan")
                mapping: "pan"
                Layout.fillWidth: true
                value: {
                    deviceRackController.revision;
                    return deviceRackController.devicePan(root.slotIndex);
                }
                onMoved: v => deviceRackController.setDevicePan(root.slotIndex, v)
            }
        }

        Text {
            text: qsTr("Devices checked here are mixed as one group and no longer reach the master on their own. Effects on this Sub Mixer apply to the whole group. Their reverb sends keep working.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 11
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
