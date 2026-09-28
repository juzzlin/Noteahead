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
import ".."

// How much of something is routed to each global send bus. The something is a whole device, or one
// pad or voice of it: a part's send is its own rather than a share of the device's, which is what
// lets one drum be in the reverb while the rest of the kit stays dry.
ColumnLayout {
    id: root
    spacing: 12

    required property string deviceName
    //! Below zero for the device itself, otherwise the pad or voice being routed.
    required property int subIndex
    //! What the part is called, for the heading. Empty for a whole device.
    property string subLabel: ""

    //! Asks for the bus's effect dialog to be opened. The bus is global, so this leaves the device.
    signal busEffectRequested(int busIndex)

    Label {
        text: root.subIndex >= 0 ? qsTr("Routing: %1 %2").arg(root.deviceName).arg(root.subLabel) : qsTr("Routing: %1").arg(root.deviceName)
        font.bold: true
        font.pointSize: 14
        color: "white"
        Layout.alignment: Qt.AlignLeft
        Layout.bottomMargin: 6
    }

    Repeater {
        model: effectRackController.masterSendCount()
        delegate: RowLayout {
            Layout.fillWidth: true
            spacing: 30
            readonly property string effectType: {
                effectRackController.revision;
                return effectRackController.masterSendEffectType(index);
            }
            visible: effectType !== ""

            // The title names the global effect this bus runs, and opens it: the send level is the
            // device's business, but what it is being sent into is not.
            Label {
                text: {
                    effectRackController.revision;
                    // The summary as the rack dialogs show it, so a bus is recognisable by what it
                    // is set to rather than only by the kind of effect it is.
                    const summary = effectRackController.masterSendParametersSummary(index);
                    return qsTr("Send %1: %2 %3").arg(index + 1).arg(effectRackController.effectDisplayName(parent.effectType)).arg(summary);
                }
                font.pointSize: 12
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                // Wide enough for an effect's name and its parameter summary; anything longer
                // elides rather than pushing the knobs out of their column.
                Layout.preferredWidth: 460
                Layout.maximumWidth: 460
                font.underline: titleMouseArea.containsMouse
                color: titleMouseArea.containsMouse ? themeService.accentColor : "white"

                MouseArea {
                    id: titleMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.busEffectRequested(index)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: containsMouse
                    ToolTip.text: qsTr("Open this send effect")
                }
            }

            // A column of its own, so every knob starts at the same place however long the effect
            // above it is named. Left to themselves the label and the knob share the row's width
            // between them, which lines the knobs up differently on every row.
            Knob {
                Layout.preferredWidth: 160
                Layout.maximumWidth: 200
                Layout.alignment: Qt.AlignVCenter
                value: {
                    effectRackController.revision;
                    return effectRackController.partSend(root.deviceName, root.subIndex, index) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setPartSend(root.deviceName, root.subIndex, index, v / Constants.uiInternalScaling)
            }

            // Everything left over, so the two columns above stay where they are however wide the
            // dialog is opened.
            Item {
                Layout.fillWidth: true
            }
        }
    }

    Label {
        text: qsTr("Nothing to send to yet: add an effect to the master send rack first.")
        color: "#aaa"
        font.italic: true
        font.pointSize: 11
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        Layout.fillWidth: true
        visible: {
            // Re-read whenever the racks change: an invokable alone is evaluated once, and this
            // sentence would then still be sitting under the sends it says do not exist.
            effectRackController.revision;
            return !effectRackController.hasMasterSendEffects();
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
