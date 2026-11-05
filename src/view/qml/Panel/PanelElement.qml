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
import "../Dialogs/ParameterAutomationTargets.js" as Targets

//! One element of the Performance Panel: a title bar naming the device or effect, with its settings
//! and a remove button, over a body chosen by the element's type.
Rectangle {
    id: root

    required property string type
    required property string sourceId

    signal removeRequested

    readonly property bool isEffect: performancePanelController.isEffectType(root.type)
    //! The device's rack slot, read again whenever the rack changes; -1 once it is gone.
    readonly property int slot: !root.isEffect && performancePanelController.revision >= 0 ? performancePanelController.deviceSlot(root.sourceId) : -1
    //! Where the effect sits, read again whenever a rack changes; empty once it is gone.
    readonly property var location: root.isEffect && performancePanelController.revision >= 0 ? performancePanelController.effectLocation(root.sourceId) : ({})
    //! Whether what the element shows is still in the project. An element of a type this version
    //! does not know is kept for the version that does, but shows nothing.
    readonly property bool found: root.isEffect ? root.location.ownerId !== undefined : root.slot >= 0

    function _title(): string {
        if (root.isEffect) {
            return root.found ? Targets.targetLabel(root.location) : qsTr("Effect missing");
        }
        return root.found ? qsTr("Slot %1: %2").arg(root.slot + 1).arg(deviceRackController.deviceDisplayName(root.slot)) : qsTr("Device missing");
    }

    function _body(): Component {
        switch (root.type) {
        case performancePanelController.xyPadType:
            return xyPadBody;
        case performancePanelController.rtaType:
            return rtaBody;
        case performancePanelController.lufsMeterType:
            return lufsMeterBody;
        case performancePanelController.dbtpMeterType:
            return dbtpMeterBody;
        case performancePanelController.stereoFieldMeterType:
            return stereoFieldBody;
        default:
            return null;
        }
    }

    //! Opens the settings of what the element shows, as its own rack would
    function _openSettings(): void {
        if (!root.isEffect) {
            deviceRackController.openDevice(root.slot);
            return;
        }
        // The rack numbers are DeviceService::EffectLocation::Rack
        const rack = root.location.rack;
        effectRackController.targetDeviceName = rack === 0 || rack === 1 ? root.location.deviceName : "";
        effectRackController.isInsertRack = rack === 0 || rack === 1 || rack === 2;
        effectRackController.targetSubIndex = rack === 1 || rack === 4 ? root.location.rackIndex : -1;
        effectDialogLauncher.open(effectRackController.effectType(root.location.effectSlot), root.location.effectSlot);
    }

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
            id: header
            Layout.fillWidth: true
            spacing: 4
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: titleLabel.implicitHeight
                Label {
                    id: titleLabel
                    text: root._title()
                    color: root.found ? Theme.textSecondary : Theme.danger
                    font.pixelSize: Theme.bodyPixelSize
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                    // Clear of the recording note in the middle
                    width: recordingLabel.visible ? Math.max(0, Math.min(parent.width, recordingLabel.x - 6 - 8)) : parent.width
                }
            }
            AppButton {
                text: "..."
                implicitWidth: 28
                implicitHeight: 20
                focusPolicy: Qt.NoFocus
                enabled: root.found
                toolTipText: qsTr("Settings")
                onClicked: root._openSettings()
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
            sourceComponent: root.found ? root._body() : null
        }
    }

    //! What a hand on the pads does while the song plays in edit mode, in the middle of the title bar
    Label {
        id: recordingLabel
        visible: root.found && root.type === performancePanelController.xyPadType && xyPadRecordingService.armed
        text: qsTr("Recording on column")
        color: Theme.danger
        font.pixelSize: Theme.bodyPixelSize
        font.bold: true
        x: (root.width - width) / 2
        y: header.y + 6 + (header.height - height) / 2
    }

    Component {
        id: rtaBody
        RtaPanelElement {
            sourceId: root.sourceId
        }
    }

    Component {
        id: lufsMeterBody
        LevelMeterPanelElement {
            sourceId: root.sourceId
            barCount: 3
        }
    }

    Component {
        id: dbtpMeterBody
        LevelMeterPanelElement {
            sourceId: root.sourceId
            barCount: 2
        }
    }

    Component {
        id: stereoFieldBody
        StereoFieldPanelElement {
            sourceId: root.sourceId
        }
    }

    Component {
        id: xyPadBody
        XyPadPanelElement {
            sourceId: root.sourceId
        }
    }
}
