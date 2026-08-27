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
    title: "<strong>" + qsTr("Effect Rack") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    property int meterPoll: 0
    Timer {
        interval: 100
        running: root.opened
        repeat: true
        onTriggered: root.meterPoll++
    }

    onOpened: {
        if (tabBar.currentIndex === 0) {
            effectRackController.isInsertRack = true;
        } else {
            effectRackController.isInsertRack = false;
        }
        effectRackController.targetDeviceName = "";
    }

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Export Rack...")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: UiService.requestExportEffectRack(tabBar.currentIndex === 0 ? qsTr("Master Insert Effects") : qsTr("Master Send Effects"))
        }
        AppButton {
            text: qsTr("Import Rack...")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: UiService.requestImportEffectRack()
        }
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Named for the rack it will actually hit. It applies to whichever tab is selected, and
            // that selector is at the bottom of the dialog, so an unqualified "Enabled" beside a
            // title reading "Master Effect Rack" invites bypassing the wrong one.
            Switch {
                text: tabBar.currentIndex === 0 ? qsTr("Insert Effects Enabled") : qsTr("Send Effects Enabled")
                checked: effectRackController.rackEnabled
                onToggled: effectRackController.rackEnabled = checked
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: tabBar.currentIndex === 0 ? qsTr("Enable or bypass the master insert chain. The send effects have their own switch on the other tab.") : qsTr("Enable or bypass every send effect. The master insert chain has its own switch on the other tab.")
            }

            Label {
                text: qsTr("Master Effect Rack")
                font.bold: true
                font.pointSize: 18
                color: "white"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
        }

        ListView {
            id: effectListView
            model: effectRackController.effectCount
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: effectListView.width
                height: 60
                color: (effectListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                readonly property string effectType: {
                    effectRackController.revision;
                    return effectRackController.effectType(index);
                }

                Menu {
                    id: manageMenu
                    delegate: MenuItemDelegate {}
                    MenuItem {
                        text: qsTr("Move Up")
                        enabled: index > 0
                        onClicked: effectRackController.moveEffectUp(index)
                    }
                    MenuItem {
                        text: qsTr("Move Down")
                        enabled: index < effectRackController.effectCount - 1
                        onClicked: effectRackController.moveEffectDown(index)
                    }
                    MenuItem {
                        text: qsTr("Move to Top")
                        enabled: index > 0
                        onClicked: effectRackController.moveEffectToTop(index)
                    }
                    MenuItem {
                        text: qsTr("Move to Bottom")
                        enabled: index < effectRackController.effectCount - 1
                        onClicked: effectRackController.moveEffectToBottom(index)
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Export Settings...")
                        onClicked: UiService.requestExportEffectSettings(index, effectType)
                    }
                    MenuItem {
                        text: qsTr("Import Settings...")
                        onClicked: UiService.requestImportEffectSettings(index)
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
                    onEntered: effectListView.hoveredIndex = index
                    onExited: effectListView.hoveredIndex = -1
                    onClicked: mouse => {
                        if (mouse.button === Qt.RightButton) {
                            if (effectType !== "") {
                                manageMenu.popup();
                            }
                            return;
                        }
                        effectListView.hoveredIndex = -1;
                        if (effectType === effectRackController.allPassFilterType) {
                            allPassFilterDialog.effectIndex = index;
                            allPassFilterDialog.open();
                        } else if (effectType === effectRackController.lufsMeterType) {
                            lufsMeterDialog.effectIndex = index;
                            lufsMeterDialog.open();
                        } else if (effectType === effectRackController.dbtpMeterType) {
                            dbtpMeterDialog.effectIndex = index;
                            dbtpMeterDialog.open();
                        } else if (effectType === effectRackController.clipperType) {
                            clipperDialog.effectIndex = index;
                            clipperDialog.open();
                        } else if (effectType === effectRackController.saturatorType) {
                            saturatorDialog.effectIndex = index;
                            saturatorDialog.open();
                        } else if (effectType === effectRackController.analogFuzzType) {
                            analogFuzzDialog.effectIndex = index;
                            analogFuzzDialog.open();
                        } else if (effectType === effectRackController.monitorType) {
                            monitorDialog.effectIndex = index;
                            monitorDialog.open();
                        } else if (effectType === effectRackController.gainType) {
                            gainDialog.effectIndex = index;
                            gainDialog.open();
                        } else if (effectType === effectRackController.bassGrinderType) {
                            bassGrinderDialog.effectIndex = index;
                            bassGrinderDialog.open();
                        } else if (effectType === effectRackController.tubeStageType) {
                            tubeStageDialog.effectIndex = index;
                            tubeStageDialog.open();
                        } else if (effectType === effectRackController.waveDesignerType) {
                            waveDesignerDialog.effectIndex = index;
                            waveDesignerDialog.open();
                        } else if (effectType === effectRackController.stereoFieldMeterType) {
                            stereoFieldMeterDialog.effectIndex = index;
                            stereoFieldMeterDialog.open();
                        } else if (effectType === effectRackController.earlyReflectionsType) {
                            earlyReflectionsDialog.effectIndex = index;
                            earlyReflectionsDialog.open();
                        } else if (effectType === effectRackController.dimensionType) {
                            dimensionDialog.effectIndex = index;
                            dimensionDialog.open();
                        } else if (effectType === effectRackController.stereoWidenerType) {
                            stereoWidenerDialog.effectIndex = index;
                            stereoWidenerDialog.open();
                        } else if (effectType === effectRackController.stereoEnhancerType) {
                            stereoEnhancerDialog.effectIndex = index;
                            stereoEnhancerDialog.open();
                        } else if (effectType === effectRackController.stereoExciterType) {
                            stereoExciterDialog.effectIndex = index;
                            stereoExciterDialog.open();
                        } else if (effectType === effectRackController.driveType) {
                            driveDialog.effectIndex = index;
                            driveDialog.open();
                        } else if (effectType === effectRackController.limiterType) {
                            limiterDialog.effectIndex = index;
                            limiterDialog.open();
                        } else if (effectType === effectRackController.compressorType) {
                            compressorDialog.effectIndex = index;
                            compressorDialog.open();
                        } else if (effectType === effectRackController.multibandCompressorType) {
                            multibandCompressorDialog.effectIndex = index;
                            multibandCompressorDialog.open();
                        } else if (effectType === effectRackController.autoDuckerType) {
                            autoDuckerDialog.effectIndex = index;
                            autoDuckerDialog.open();
                        } else if (effectType === effectRackController.delayType) {
                            delayDialog.effectIndex = index;
                            delayDialog.open();
                        } else if (effectType === effectRackController.eq8BandParametricType) {
                            eq8BandParametricDialog.effectIndex = index;
                            eq8BandParametricDialog.open();
                        } else if (effectType === effectRackController.vintagePassiveEqType) {
                            vintagePassiveEqDialog.effectIndex = index;
                            vintagePassiveEqDialog.open();
                        } else if (effectType === effectRackController.airBandEqType) {
                            airBandEqDialog.effectIndex = index;
                            airBandEqDialog.open();
                        } else if (effectType === effectRackController.simpleEqType) {
                            simpleEqDialog.effectIndex = index;
                            simpleEqDialog.open();
                        } else if (effectType === effectRackController.pannerType) {
                            pannerDialog.effectIndex = index;
                            pannerDialog.open();
                        } else if (effectType === effectRackController.autoPannerType) {
                            autoPannerDialog.effectIndex = index;
                            autoPannerDialog.open();
                        } else if (effectType === effectRackController.autoFilterType) {
                            autoFilterDialog.effectIndex = index;
                            autoFilterDialog.open();
                        } else if (effectType === effectRackController.phaserType) {
                            phaserDialog.effectIndex = index;
                            phaserDialog.open();
                        } else if (effectType === effectRackController.chorusType) {
                            chorusDialog.effectIndex = index;
                            chorusDialog.open();
                        } else if (effectType === effectRackController.reverbType) {
                            reverbDialog.effectIndex = index;
                            reverbDialog.open();
                        } else if (effectType === effectRackController.endlessType) {
                            endlessReverbDialog.effectIndex = index;
                            endlessReverbDialog.open();
                        } else if (effectType === effectRackController.rtaType) {
                            rtaDialog.effectIndex = index;
                            rtaDialog.open();
                        } else if (effectType === "") {
                            UiService.requestEffectsGalleryDialog(index);
                        } else {
                            UiService.requestEffectsGalleryDialog(index);
                        }
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    Text {
                        text: {
                            effectRackController.revision;
                            root.meterPoll;
                            if (effectType === "") return "";
                            const name = effectRackController.effectDisplayName(effectType);
                            const summary = effectRackController.effectParametersSummary(index);
                            return qsTr("Slot %1: %2 %3").arg(index + 1).arg(name).arg(summary);
                        }
                        color: "white"
                        font.pointSize: 13
                        font.bold: effectListView.hoveredIndex === index && root.activeFocus
                        // The controls on the row have fixed widths, so the summary is what gives
                        // way on a narrow rack instead of running over them
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        elide: Text.ElideRight
                        visible: effectType !== ""
                    }
                    Image {
                        source: "../Graphics/add_box.png"
                        sourceSize.width: 32
                        sourceSize.height: 32
                        Layout.alignment: Qt.AlignCenter
                        visible: effectType === ""
                        opacity: (effectListView.hoveredIndex === index && root.activeFocus) ? 1.0 : 0.5
                    }
                    CheckBox {
                        visible: effectType !== ""
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        checked: {
                            effectRackController.revision;
                            return effectRackController.isEffectEnabled(index);
                        }
                        onToggled: effectRackController.setIsEffectEnabled(index, checked)
                        ToolTip.visible: hovered
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.text: qsTr("Enable/disable the effect")
                    }
                    AppButton {
                        id: manageButton
                        text: qsTr("Manage")
                        Layout.preferredWidth: 80
                        visible: effectType !== ""
                        onClicked: manageMenu.popup(manageButton, 0, manageButton.height)
                    }
                    AppButton {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        visible: effectType !== ""
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
                        onClicked: effectRackController.clearEffect(index)
                    }
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Universal.theme: Universal.Dark
            TabButton {
                text: qsTr("Insert Effects")
                onClicked: {
                    effectRackController.isInsertRack = true;
                    effectRackController.targetDeviceName = "";
                }
            }
            TabButton {
                text: qsTr("Send Effects")
                onClicked: {
                    effectRackController.isInsertRack = false;
                    effectRackController.targetDeviceName = "";
                }
            }
        }

        Text {
            text: tabBar.currentIndex === 0 ? qsTr("Insert effects are processed in order. Dry/Wet mix is handled by each effect.") : qsTr("To route an internal instrument to a send effect, use the Sends button in the Device Rack dialog.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 11
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
