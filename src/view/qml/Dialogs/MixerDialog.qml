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

// One row per populated device slot: the mixing controls of every device in one place. The knobs
// drive the very same device parameters the device dialogs do, so nothing is mirrored or cached
// here -- opening a device shows whatever was set from the mixer, and the other way round.
AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Mixer") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 640

    // The level taps are a no-op until switched on, so they only run while the mixer is on screen.
    onVisibleChanged: deviceRackController.setMetersActive(visible)

    readonly property int populatedCount: {
        deviceRackController.revision;
        return deviceRackController.populatedDevices().length;
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Clear clips")
            // The one button in the dialog whose label runs past the standard width in most
            // translations, so it takes whatever its own text needs and the standard width is
            // only the floor. ActionRole keeps the dialog open, which is the point of a button
            // that resets the meters while they are being watched.
            implicitWidth: Math.max(Constants.defaultButtonWidth, implicitContentWidth + leftPadding + rightPadding)
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            toolTipText: qsTr("Reset every clip indicator in the mixer")
            onClicked: {
                deviceRackController.clearAllDeviceClips();
                mainLayout.meterTick++;
            }
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Bumped by the timer below; the meter bindings depend on it, which is what makes them
        // re-read the taps without each row owning a timer of its own.
        property int meterTick: 0

        Timer {
            interval: 50
            running: root.visible
            repeat: true
            onTriggered: mainLayout.meterTick++
        }

        Label {
            text: qsTr("Mixer")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("No devices yet. Add them in the Device Rack.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 12
            visible: root.populatedCount === 0
            Layout.alignment: Qt.AlignHCenter
        }

        ScrollView {
            id: mixerScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: mixerScrollView.availableWidth
                spacing: 10

                // The slot count is the model rather than the list of populated devices: that list
                // is rebuilt whenever a knob moves, which would tear down the delegate mid-drag.
                // Empty slots are simply left out of the layout instead.
                Repeater {
                    model: deviceRackController.deviceCount

                    delegate: Rectangle {
                        id: strip
                        Layout.fillWidth: true
                        Layout.preferredHeight: 76
                        visible: deviceType !== ""
                        color: "#333"
                        radius: 5
                        border.color: "#555"

                        readonly property string deviceType: {
                            deviceRackController.revision;
                            return deviceRackController.deviceType(index);
                        }
                        readonly property string deviceTypeName: {
                            deviceRackController.revision;
                            return deviceRackController.deviceTypeName(index);
                        }
                        readonly property string deviceName: {
                            deviceRackController.revision;
                            return deviceRackController.deviceName(index);
                        }
                        readonly property string trackNames: {
                            deviceRackController.revision;
                            return deviceRackController.trackNames(index);
                        }
                        readonly property var meterLevels: {
                            mainLayout.meterTick;
                            deviceRackController.revision;
                            return deviceRackController.deviceMeterLevels(index);
                        }
                        readonly property bool deviceClipped: {
                            mainLayout.meterTick;
                            deviceRackController.revision;
                            return deviceRackController.deviceClipped(index);
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            ColumnLayout {
                                Layout.preferredWidth: 180
                                spacing: 2

                                Text {
                                    text: qsTr("%1: %2").arg(index + 1).arg(strip.deviceName)
                                    color: "white"
                                    font.pointSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: strip.trackNames === "" ? strip.deviceTypeName : qsTr("%1 — %2").arg(strip.deviceTypeName).arg(strip.trackNames)
                                    color: "#aaa"
                                    font.pointSize: 10
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            // Each setter reaches revisionChanged via Device::dataChanged, so
                            // referencing revision is what makes these bindings re-evaluate -- both
                            // after a drag and when MIDI CC moves a value underneath them.
                            Knob {
                                label: qsTr("Gain")
                                mapping: "decibel"
                                mapMin: -30
                                mapMax: 30
                                Layout.preferredWidth: 130
                                value: {
                                    deviceRackController.revision;
                                    return deviceRackController.deviceGain(index);
                                }
                                onMoved: v => deviceRackController.setDeviceGain(index, v)
                            }

                            Knob {
                                label: qsTr("Fader")
                                mapping: "fader"
                                Layout.preferredWidth: 130
                                value: {
                                    deviceRackController.revision;
                                    return deviceRackController.deviceVolume(index);
                                }
                                onMoved: v => deviceRackController.setDeviceVolume(index, v)
                            }

                            Knob {
                                label: qsTr("Pan")
                                mapping: "pan"
                                Layout.preferredWidth: 130
                                value: {
                                    deviceRackController.revision;
                                    return deviceRackController.devicePan(index);
                                }
                                onMoved: v => deviceRackController.setDevicePan(index, v)
                            }

                            LevelMeterBar {
                                Layout.preferredWidth: 110
                                Layout.alignment: Qt.AlignVCenter
                                peakDb: strip.meterLevels.length ? strip.meterLevels[0] : -120
                                rmsDb: strip.meterLevels.length ? strip.meterLevels[1] : -120
                                markerDb: settingsService.gainStagingTargetDb
                            }

                            ClipLed {
                                Layout.alignment: Qt.AlignVCenter
                                clipped: strip.deviceClipped
                                onClicked: {
                                    deviceRackController.clearDeviceClip(index);
                                    mainLayout.meterTick++;
                                }
                            }

                            AppButton {
                                text: qsTr("Device")
                                Layout.preferredWidth: 80
                                toolTipText: qsTr("Open the device dialog")
                                onClicked: deviceRackController.openDevice(index)
                            }

                            AppButton {
                                text: qsTr("Insert FX")
                                Layout.preferredWidth: 80
                                toolTipText: qsTr("Insert effects of this device")
                                onClicked: UiService.requestDeviceInsertEffectsDialog(strip.deviceName)
                            }

                            AppButton {
                                text: qsTr("Sends")
                                Layout.preferredWidth: 80
                                toolTipText: qsTr("Send levels of this device to the master send effects")
                                onClicked: UiService.requestEffectSendsDialog(strip.deviceName)
                            }
                        }
                    }
                }
            }
        }

        Text {
            text: qsTr("These are the very same controls as in each device's own dialog: Gain trims the device into its insert effects, the Fader balances it, Pan places it.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 11
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }
}
