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
import QtQuick.Window 2.15
import Noteahead 1.0
import "../Components"
import "ParameterAutomationTargets.js" as Targets

//! Picks what a modulation source moves: the device or effect, then the parameter, from the same
//! list automation offers. Shared by the LFO device's and the XY pad's dialogs.
ColumnLayout {
    id: root

    //! Everything a parameter automation can drive, as propertyService.automationTargets() lists it.
    required property var targets
    //! The current target, as the panel controller hands it over; an empty owner id is none.
    required property string owner
    required property string ownerId
    required property string parameter
    //! Whether another modulation source moves the same parameter.
    property bool shared: false
    //! What the bottom and the top of the swing send, for a MIDI CC target.
    property int ccMin: 0
    property int ccMax: 127
    //! Whether the target is a MIDI controller rather than a parameter: then Min and Max say how
    //! far it goes, instead of a depth around a setting.
    readonly property bool isMidiCc: root.owner === "midiCc" && root.ownerId !== ""
    property string label: qsTr("Target")

    signal targetChosen(string owner, string ownerId, string parameter)
    //! A device or effect was chosen, with its parameters in the order they are listed: which of
    //! them to take is for the source's controller to decide, from what is already taken.
    signal ownerChosen(string owner, string ownerId, var parameters)
    signal targetCleared
    signal ccMinChosen(int value)
    signal ccMaxChosen(int value)

    readonly property int targetIndex: root.ownerId === "" ? -1 : Targets.findTargetIndex(root.targets, root.owner, root.ownerId, root.parameter)
    readonly property var target: root.targetIndex >= 0 ? root.targets[root.targetIndex] : null

    Layout.fillWidth: true
    spacing: 6

    GridLayout {
        id: grid
        // Two thirds of the row for the target, sized from the picker rather than left to the layout,
        // which shares out width by what each box holds: a target's name runs long, a parameter's
        // does not
        readonly property real share: (root.width - columnSpacing) / 3
        columns: 2
        columnSpacing: 10
        Layout.fillWidth: true
        Label {
            text: root.label
        }
        Label {
            text: qsTr("Parameter")
        }
        ComboBox {
            id: targetComboBox
            model: [qsTr("None")].concat(root.targets.map(target => Targets.targetLabel(target)))
            // The labels name the device and the tracks it plays on, so they run long: the list
            // opens as wide as its longest entry, as far as the window allows, rather than eliding
            // them all to the box's width
            popup.width: Math.max(width, Math.min(longestLabelWidth + 2 * edgeMargin + leftPadding + rightPadding, Window.width - 2 * edgeMargin))
            popup.leftMargin: edgeMargin
            popup.rightMargin: edgeMargin
            readonly property int edgeMargin: 10
            readonly property real longestLabelWidth: model.reduce((widest, label) => Math.max(widest, labelMetrics.advanceWidth(label)), 0)
            FontMetrics {
                id: labelMetrics
                font: targetComboBox.font
            }
            ToolTip.visible: hovered && currentIndex > 0 && contentItem.truncated === true
            ToolTip.text: displayText
            ToolTip.delay: Constants.toolTipDelay
            // A target that is no longer in the project reads as nothing chosen rather than as
            // something else
            currentIndex: root.targetIndex + 1
            onActivated: index => {
                if (index === 0) {
                    root.targetCleared();
                } else {
                    const target = root.targets[index - 1];
                    root.ownerChosen(target.owner, target.ownerId, target.parameters.map(parameter => parameter.key));
                }
            }
            Layout.fillWidth: true
            Layout.preferredWidth: grid.share * 2
        }
        ComboBox {
            enabled: root.target !== null
            model: root.target ? root.target.parameters.map(parameter => parameter.label) : []
            currentIndex: root.target ? root.target.parameters.findIndex(parameter => parameter.key === root.parameter) : -1
            onActivated: index => root.targetChosen(root.target.owner, root.target.ownerId, root.target.parameters[index].key)
            Layout.fillWidth: true
            Layout.preferredWidth: grid.share
        }
    }

    RowLayout {
        visible: root.isMidiCc
        spacing: 10
        Layout.fillWidth: true
        Label {
            text: qsTr("Min")
        }
        SpinBox {
            from: 0
            to: 127
            editable: true
            value: root.ccMin
            onValueModified: root.ccMinChosen(value)
            ToolTip.visible: hovered
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.text: qsTr("What the bottom of the swing sends")
        }
        Label {
            text: qsTr("Max")
        }
        SpinBox {
            from: 0
            to: 127
            editable: true
            value: root.ccMax
            onValueModified: root.ccMaxChosen(value)
            ToolTip.visible: hovered
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.text: qsTr("What the top of the swing sends. Below Min turns it around.")
        }
        Item {
            Layout.fillWidth: true
        }
    }

    // Missing rather than silently None: a source whose target was deleted still has it saved
    Label {
        visible: root.ownerId !== "" && root.targetIndex < 0
        text: qsTr("Target missing")
        color: Theme.danger
        font.pixelSize: Theme.bodyPixelSize
    }
    Label {
        visible: root.shared && root.targetIndex >= 0
        text: root.isMidiCc ? qsTr("Also sent by another modulator: the last value sent wins") : qsTr("Also moved by another modulator: their movements add up")
        color: Theme.warning
        font.pixelSize: Theme.bodyPixelSize
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
