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
import ".."
import "../Components"

//! One element of the Performance Panel: a title bar naming the device, with its settings and a
//! remove button, over a body chosen by the element's type.
Rectangle {
    id: root

    required property string type
    required property string sourceId

    signal removeRequested

    //! The device's rack slot, read again whenever the rack changes; -1 once it is gone.
    readonly property int slot: performancePanelController.revision >= 0 ? performancePanelController.deviceSlot(root.sourceId) : -1

    implicitWidth: Math.max(body.item ? body.item.implicitWidth : 0, 180) + 12
    width: implicitWidth
    color: Theme.surfaceInset
    border.color: Theme.borderSubtle
    radius: 2

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Label {
                text: root.slot >= 0 ? qsTr("Slot %1: %2").arg(root.slot + 1).arg(deviceRackController.deviceDisplayName(root.slot)) : qsTr("Device missing")
                color: root.slot >= 0 ? Theme.textSecondary : Theme.danger
                font.pixelSize: Theme.bodyPixelSize
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            AppButton {
                text: "..."
                implicitWidth: 28
                implicitHeight: 20
                focusPolicy: Qt.NoFocus
                enabled: root.slot >= 0
                toolTipText: qsTr("Settings")
                onClicked: deviceRackController.openDevice(root.slot)
            }
            AppButton {
                text: "×"
                implicitWidth: 28
                implicitHeight: 20
                focusPolicy: Qt.NoFocus
                toolTipText: qsTr("Remove from the panel")
                onClicked: root.removeRequested()
            }
        }

        Loader {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: root.slot < 0 ? null : root.type === performancePanelController.xyPadType ? xyPadBody : null
        }
    }

    Component {
        id: xyPadBody
        XyPadPanelElement {
            sourceId: root.sourceId
        }
    }
}
