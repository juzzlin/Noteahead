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
    property string deviceName: ""
    title: "<strong>" + qsTr("Effect Sends: ") + deviceName + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.defaultDialogScale : 600
    height: parent ? parent.height * Constants.defaultDialogScale : 500

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    onOpened: {
        effectRackController.isInsertRack = false;
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

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: 20

            Label {
                text: qsTr("Routing: ") + deviceName
                font.bold: true
                font.pointSize: 16
                color: "white"
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            Repeater {
                model: effectRackController.effectCount
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 30
                    readonly property string effectType: {
                        effectRackController.revision;
                        return effectRackController.effectType(index);
                    }
                    visible: effectType !== ""
                    Label {
                        text: {
                            effectRackController.revision;
                            const summary = effectRackController.effectParametersSummary(index);
                            return qsTr("Send %1: %2 %3").arg(index + 1).arg(effectRackController.effectDisplayName(effectType)).arg(summary);
                        }
                        font.pointSize: 12
                        Layout.fillWidth: true
                    }
                    Knob {
                        value: {
                            effectRackController.revision;
                            return effectRackController.deviceSend(root.deviceName, index) * Constants.uiInternalScaling;
                        }
                        onMoved: v => effectRackController.setDeviceSend(root.deviceName, index, v / Constants.uiInternalScaling)
                    }
                }
            }
            
            Item { Layout.fillHeight: true }
        }
    }
}
