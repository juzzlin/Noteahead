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
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    //! What a mapping can move, as { key, label }
    property var mappingTargets: []
    //! The mapping row waiting for a controller to be moved, or -1
    property int learningRow: -1

    onOpened: {
        root.slotIndex = deviceRackController.slotOfDevice(root.deviceName);
        faderPositionCombo.currentIndex = deviceMixerController.deviceFaderPosition(root.slotIndex);
        sendTapCombo.currentIndex = deviceMixerController.deviceSendTap(root.slotIndex);
        root.learningRow = -1;
        root.mappingTargets = propertyService.midiMappingTargets(root.slotIndex);
        mappingsModel.clear();
        for (const mapping of midiMappingController.deviceMidiMappings(root.slotIndex)) {
            mappingsModel.append(mapping);
        }
    }
    onClosed: root.learningRow = -1

    function commitMappings(): void {
        const mappings = [];
        for (let i = 0; i < mappingsModel.count; i++) {
            const mapping = mappingsModel.get(i);
            mappings.push({
                "controller": mapping.controller,
                "target": mapping.target,
                "min": mapping.min,
                "max": mapping.max
            });
        }
        midiMappingController.setDeviceMidiMappings(root.slotIndex, mappings);
    }

    //! In the dialog rather than in the row: removing the row destroys the context the row's own
    //! handler runs in, and anything after the removal there would no longer resolve.
    function removeMapping(row: int): void {
        if (root.learningRow === row) {
            root.learningRow = -1;
        } else if (root.learningRow > row) {
            root.learningRow--;
        }
        mappingsModel.remove(row);
        root.commitMappings();
    }

    ListModel {
        id: mappingsModel
    }

    Connections {
        target: midiMappingController
        function onControllerCcReceived(controller: int): void {
            if (root.visible && root.learningRow >= 0 && root.learningRow < mappingsModel.count && controller <= midiMappingController.maxMappableMidiCc()) {
                mappingsModel.setProperty(root.learningRow, "controller", controller);
                root.learningRow = -1;
                root.commitMappings();
            }
        }
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 14

            SectionHeading {
                text: qsTr("Signal Flow")
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
                    onActivated: deviceMixerController.setDeviceFaderPosition(root.slotIndex, currentIndex)
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

            SectionHeading {
                text: qsTr("Effect Sends")
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
                    onActivated: deviceMixerController.setDeviceSendTap(root.slotIndex, currentIndex)
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

            SectionHeading {
                text: qsTr("MIDI Mappings")
                Layout.topMargin: 10
            }

            Label {
                text: qsTr("A controller mapped here moves the parameter between Min and Max, ahead of whatever the device itself does with that controller. Press Learn and move a knob on the controller to pick its CC.")
                font.pixelSize: 12
                opacity: 0.7
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: mappingsModel
                delegate: ColumnLayout {
                    id: mappingRow
                    required property int index
                    required property int controller
                    required property string target
                    required property double min
                    required property double max
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            text: qsTr("CC")
                            Layout.preferredWidth: 40
                        }
                        SpinBox {
                            from: 0
                            to: midiMappingController.maxMappableMidiCc()
                            value: mappingRow.controller
                            editable: true
                            Layout.preferredWidth: 120
                            onValueModified: {
                                mappingsModel.setProperty(mappingRow.index, "controller", value);
                                root.commitMappings();
                            }
                        }
                        AppButton {
                            text: root.learningRow === mappingRow.index ? qsTr("Move...") : qsTr("Learn")
                            highlighted: root.learningRow === mappingRow.index
                            toolTipText: qsTr("Take the CC from the next controller knob you move")
                            onClicked: root.learningRow = root.learningRow === mappingRow.index ? -1 : mappingRow.index
                        }
                        ComboBox {
                            id: mappingTargetComboBox
                            model: root.mappingTargets
                            popup: ColumnsPopup {
                                comboBox: mappingTargetComboBox
                            }
                            textRole: "label"
                            valueRole: "key"
                            Layout.fillWidth: true
                            Component.onCompleted: currentIndex = indexOfValue(mappingRow.target)
                            onActivated: {
                                mappingsModel.setProperty(mappingRow.index, "target", currentValue);
                                root.commitMappings();
                            }
                        }
                        AppButton {
                            text: "\u2715"
                            implicitWidth: 40
                            toolTipText: qsTr("Remove this mapping")
                            onClicked: root.removeMapping(mappingRow.index)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            text: qsTr("Min")
                            Layout.preferredWidth: 40
                        }
                        SpinBox {
                            from: 0
                            to: 100
                            value: Math.round(mappingRow.min * 100)
                            editable: true
                            Layout.preferredWidth: 120
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Knob position, in percent, at the controller's bottom")
                            onValueModified: {
                                mappingsModel.setProperty(mappingRow.index, "min", value / 100);
                                root.commitMappings();
                            }
                        }
                        Label {
                            text: qsTr("Max")
                            Layout.leftMargin: 12
                        }
                        SpinBox {
                            from: 0
                            to: 100
                            value: Math.round(mappingRow.max * 100)
                            editable: true
                            Layout.preferredWidth: 120
                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Knob position, in percent, at the controller's top")
                            onValueModified: {
                                mappingsModel.setProperty(mappingRow.index, "max", value / 100);
                                root.commitMappings();
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            AppButton {
                text: qsTr("Add Mapping")
                enabled: root.mappingTargets.length > 0
                toolTipText: qsTr("Map a controller to a parameter of this device")
                onClicked: {
                    mappingsModel.append({
                        "controller": 1,
                        "target": root.mappingTargets[0].key,
                        "min": 0.0,
                        "max": 1.0
                    });
                    root.commitMappings();
                }
            }
        }
    }
}
