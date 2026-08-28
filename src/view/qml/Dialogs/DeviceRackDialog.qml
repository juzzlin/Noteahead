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
    title: "<strong>" + qsTr("Device Rack") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    function updateUsage(): void {
        deviceRackController.refresh();
    }

    // The level taps are a no-op until switched on, so they only run while the rack is on screen.
    onVisibleChanged: deviceRackController.setMetersActive(visible)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Timer {
            interval: 50
            running: root.visible
            repeat: true
            onTriggered: deviceListView.meterTick++
        }

        Label {
            text: qsTr("Device Rack")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: deviceListView
            model: deviceRackController.deviceCount
            property int hoveredIndex: -1
            // Bumped by the timer below; the delegates' level bindings depend on it, which is what
            // makes them re-read the meters without each row owning a timer of its own.
            property int meterTick: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: deviceListView.width
                height: 60
                color: (deviceListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
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
                    deviceListView.meterTick;
                    deviceRackController.revision;
                    return deviceRackController.deviceMeterLevels(index);
                }
                readonly property real deviceLoad: {
                    deviceListView.meterTick;
                    deviceRackController.revision;
                    return deviceRackController.deviceLoad(index);
                }
                readonly property bool deviceClipped: {
                    deviceListView.meterTick;
                    deviceRackController.revision;
                    return deviceRackController.deviceClipped(index);
                }

                Menu {
                    id: manageMenu
                    delegate: MenuItemDelegate {}
                    MenuItem {
                        text: qsTr("Change Device...")
                        onClicked: UiService.requestDeviceGalleryDialog(index)
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Export Settings...")
                        onClicked: UiService.requestExportDeviceSettings(index, deviceName, deviceTypeName)
                    }
                    MenuItem {
                        text: qsTr("Import Settings...")
                        onClicked: UiService.requestImportDeviceSettings(index)
                    }
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    // The buttons on the row take the left button themselves, but they do not want
                    // the right one, so it falls through to here wherever it is pressed on the row
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onEntered: deviceListView.hoveredIndex = index
                    onExited: deviceListView.hoveredIndex = -1
                    onClicked: mouse => {
                        if (mouse.button === Qt.RightButton) {
                            if (deviceType !== "") {
                                manageMenu.popup();
                            }
                            return;
                        }
                        deviceListView.hoveredIndex = -1;
                        if (deviceType === "") {
                            UiService.requestDeviceGalleryDialog(index);
                        } else {
                            deviceRackController.openDevice(index);
                        }
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    Text {
                        text: deviceType === "" ? "" : qsTr("Slot %1: %2 (%3)").arg(index + 1).arg(deviceName).arg(deviceTypeName)
                        color: "white"
                        font.pointSize: 13
                        font.bold: deviceListView.hoveredIndex === index && root.activeFocus
                        // The buttons and meters on the row have fixed widths, so the name is what
                        // gives way on a narrow rack instead of running over them
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        elide: Text.ElideRight
                        visible: deviceType !== ""
                    }

                    Text {
                        text: trackNames
                        color: "#aaa"
                        font.pointSize: 11
                        Layout.preferredWidth: 150
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                        visible: deviceType !== ""
                    }

                    Image {
                        source: "../Graphics/add_box.png"
                        sourceSize.width: 32
                        sourceSize.height: 32
                        Layout.alignment: Qt.AlignCenter
                        visible: deviceType === ""
                        opacity: (deviceListView.hoveredIndex === index && root.activeFocus) ? 1.0 : 0.5
                    }

                    LevelMeterBar {
                        Layout.preferredWidth: 110
                        Layout.alignment: Qt.AlignVCenter
                        visible: deviceType !== ""
                        peakDb: meterLevels.length ? meterLevels[0] : -120
                        rmsDb: meterLevels.length ? meterLevels[1] : -120
                        markerDb: settingsService.gainStagingTargetDb
                    }

                    ClipLed {
                        Layout.alignment: Qt.AlignVCenter
                        visible: deviceType !== ""
                        clipped: deviceClipped
                        onClicked: {
                            deviceRackController.clearDeviceClip(index);
                            deviceListView.meterTick++;
                        }
                    }

                    Text {
                        text: deviceLoad.toFixed(1) + " %"
                        // Amber past a third of the budget, red past two thirds: one device eating
                        // that much of a callback is what tips a busy song over.
                        color: deviceLoad > 66 ? "#ff6060" : (deviceLoad > 33 ? "#d0a040" : "#aaa")
                        font.pointSize: 11
                        Layout.preferredWidth: 55
                        horizontalAlignment: Text.AlignRight
                        visible: deviceType !== ""
                    }

                    AppButton {
                        text: qsTr("Insert FX")
                        onClicked: UiService.requestDeviceInsertEffectsDialog(deviceName)
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                    }

                    AppButton {
                        text: qsTr("Sends")
                        onClicked: UiService.requestEffectSendsDialog(deviceName)
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                    }

                    AppButton {
                        text: qsTr("Settings")
                        onClicked: UiService.requestDeviceSettingsDialog(deviceName)
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                    }

                    AppButton {
                        id: manageButton
                        text: qsTr("Manage")
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                        onClicked: manageMenu.popup(manageButton, 0, manageButton.height)
                    }

                    AppButton {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        visible: deviceType !== ""
                        flat: true
                        padding: 0
                        Image {
                            source: "../Graphics/delete.png"
                            anchors.fill: parent
                            anchors.margins: 4
                            sourceSize.width: 24
                            sourceSize.height: 24
                            fillMode: Image.PreserveAspectFit
                            opacity: parent.hovered ? 1.0 : 0.6
                        }
                        onClicked: deviceRackController.clearDevice(index)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Text {
                // The tick has to be read inside each binding: a property declared on the parent
                // creates no dependency here, so these would evaluate once and never update.
                readonly property real load: {
                    deviceListView.meterTick;
                    return deviceRackController.totalLoad();
                }
                readonly property real peak: {
                    deviceListView.meterTick;
                    return deviceRackController.totalPeakLoad();
                }
                text: qsTr("DSP load: %1 % (peak %2 %)").arg(load.toFixed(1)).arg(peak.toFixed(1))
                color: peak > 90 ? "#ff6060" : "#aaa"
                font.pointSize: 11
            }

            Text {
                readonly property int dropouts: {
                    deviceListView.meterTick;
                    return deviceRackController.overrunCount();
                }
                text: qsTr("Dropouts: %1").arg(dropouts)
                color: dropouts > 0 ? "#ff6060" : "#aaa"
                font.pointSize: 11
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Text {
            text: qsTr("To assign an internal device to a track, select its name from the port list in the Track Settings dialog.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 11
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
