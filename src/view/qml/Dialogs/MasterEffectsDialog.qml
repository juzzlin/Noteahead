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

Dialog {
    id: root
    title: "<strong>" + qsTr("Effect Rack") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

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
        Button {
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
            text: qsTr("Master Effect Rack")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
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
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: effectListView.hoveredIndex = index
                    onExited: effectListView.hoveredIndex = -1
                    onClicked: {
                        effectListView.hoveredIndex = -1;
                        if (effectType === "reverb") {
                            reverbDialog.effectIndex = index;
                            reverbDialog.open();
                        } else if (effectType === "compressor") {
                            compressorDialog.effectIndex = index;
                            compressorDialog.open();
                        } else if (effectType === "clipper") {
                            clipperDialog.effectIndex = index;
                            clipperDialog.open();
                        } else if (effectType === "eq8bandparametric") {
                            eq8BandParametricDialog.effectIndex = index;
                            eq8BandParametricDialog.open();
                        } else if (effectType === "panner") {
                            pannerDialog.effectIndex = index;
                            pannerDialog.open();
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
                            if (effectType === "") return "";
                            let name = effectType;
                            if (effectType === "eq8bandparametric") {
                                name = "EQ 8-Band Parametric";
                            } else {
                                name = effectType.charAt(0).toUpperCase() + effectType.slice(1);
                            }
                            const summary = effectRackController.effectParametersSummary(index);
                            return qsTr("Slot %1: %2 %3").arg(index + 1).arg(name).arg(summary);
                        }
                        color: "white"
                        font.pointSize: 13
                        font.bold: effectListView.hoveredIndex === index && root.activeFocus
                        Layout.fillWidth: true
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
                    Button {
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
