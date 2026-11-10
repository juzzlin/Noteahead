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
import "ParameterAutomationTargets.js" as Targets

//! The XY pad device: its four pads, to play, and what each pad's axes move.
AnimatedDialog {
    id: root
    title: xyPadController.displayName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 700

    //! The name of what pad @p pad moves on @p axis (0 for X, 1 for Y), or empty for nothing.
    function axisLabel(pad, axis) {
        const owner = axis === 0 ? pad.xTargetOwner : pad.yTargetOwner;
        const ownerId = axis === 0 ? pad.xTargetOwnerId : pad.yTargetOwnerId;
        const key = axis === 0 ? pad.xTargetParameter : pad.yTargetParameter;
        const parameter = ownerId === "" ? null : Targets.findParameter(Targets.findTarget(root.targets, owner, ownerId, key), key);
        return parameter ? parameter.label : "";
    }

    //! Read when the dialog opens: what there is to modulate changes as devices and effects come and go
    property var targets: []
    onAboutToShow: {
        targets = propertyService.modulationTargets();
        // Another device's targets may have moved since: the shared warnings read the whole rack
        xyPadController.requestSettings();
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: xyPadController
            hostDialog: root
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => xyPadController.accept()
        onRejected: () => xyPadController.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // The pads to play, all four in a row as the performance panel has them, and out of the
        // scrolling area below so that they are always at hand
        RowLayout {
            id: padRow
            spacing: 15
            Layout.alignment: Qt.AlignHCenter
            // Square, but never more than a third of the dialog: the settings need the rest. From the
            // dialog's width, not the row's, which follows from this.
            readonly property real padSize: Math.min((root.availableWidth - 30 - spacing * (xyPadController.padCount - 1)) / xyPadController.padCount, root.availableHeight / 3)
            Layout.preferredHeight: padSize
            Layout.minimumHeight: padSize

            Repeater {
                model: xyPadController.pads
                delegate: XyPadRenderer {
                    required property var modelData
                    controller: xyPadController
                    pad: modelData.index
                    xLabel: root.axisLabel(modelData, 0)
                    yLabel: root.axisLabel(modelData, 1)
                    accentColor: themeService.accentColor
                    Layout.preferredWidth: padRow.padSize
                    Layout.preferredHeight: padRow.padSize
                }
            }
        }

        ScrollView {
            id: contentScrollView
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Two by two, as the LFO device's dialog: four columns would leave the pickers too narrow
            GridLayout {
                width: contentScrollView.availableWidth
                columns: 2
                columnSpacing: 30
                rowSpacing: 20

                Repeater {
                    model: xyPadController.pads
                    delegate: XyPadDialog_Pad {
                        required property var modelData
                        pad: modelData
                        targets: root.targets
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                    }
                }
            }
        }
    }
}
