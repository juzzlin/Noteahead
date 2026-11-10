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
    //! What the bottom and the top of the swing send, for a MIDI target: a CC's value, or the pitch
    //! bend in percent.
    property int midiMin: 0
    property int midiMax: 127
    //! Whether the MIDI target is a pitch bend, whose Min and Max run from -100 % to 100 %.
    property bool pitchBend: false
    //! Whether the target is a MIDI controller or pitch bend rather than a parameter: then Min and
    //! Max say how far it goes, instead of a depth around a setting.
    readonly property bool isMidi: root.owner === "midi" && root.ownerId !== ""
    property string label: qsTr("Target")
    //! Whether Min and Max go on the target's own row, for a picker given the room
    property bool rangeBesideTarget: false
    readonly property bool showsRangeBeside: root.isMidi && root.rangeBesideTarget

    signal targetChosen(string owner, string ownerId, string parameter)
    //! A device or effect was chosen, with its parameters in the order they are listed: which of
    //! them to take is for the source's controller to decide, from what is already taken.
    signal ownerChosen(string owner, string ownerId, var parameters)
    signal targetCleared
    signal midiMinChosen(int value)
    signal midiMaxChosen(int value)

    readonly property int targetIndex: root.ownerId === "" ? -1 : Targets.findTargetIndex(root.targets, root.owner, root.ownerId, root.parameter)
    readonly property var target: root.targetIndex >= 0 ? root.targets[root.targetIndex] : null

    Layout.fillWidth: true
    spacing: 6

    //! A CC's value, 0 to 127, or a pitch bend in percent, -100 % to 100 %
    component MidiRangeSpinBox: SpinBox {
        property bool pitchBend: false
        from: pitchBend ? -100 : 0
        to: pitchBend ? 100 : 127
        editable: true
        textFromValue: (value, locale) => pitchBend ? value + " %" : value.toString()
        valueFromText: (text, locale) => parseInt(text) || 0
    }

    component MinSpinBox: MidiRangeSpinBox {
        pitchBend: root.pitchBend
        value: root.midiMin
        onValueModified: root.midiMinChosen(value)
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("What the bottom of the swing sends")
    }

    component MaxSpinBox: MidiRangeSpinBox {
        pitchBend: root.pitchBend
        value: root.midiMax
        onValueModified: root.midiMaxChosen(value)
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("What the top of the swing sends. Below Min turns it around.")
    }

    GridLayout {
        id: grid
        // Two thirds of the row for the target, sized from the picker rather than left to the layout,
        // which shares out width by what each box holds: a target's name runs long, a parameter's
        // does not
        readonly property real share: (root.width - (columns - 1) * columnSpacing - (root.showsRangeBeside ? 2 * rangeWidth : 0)) / 3
        readonly property real rangeWidth: 130
        columns: root.showsRangeBeside ? 4 : 2
        columnSpacing: 10
        Layout.fillWidth: true
        Label {
            text: root.label
        }
        Label {
            text: qsTr("Parameter")
        }
        Label {
            visible: root.showsRangeBeside
            text: qsTr("Min")
        }
        Label {
            visible: root.showsRangeBeside
            text: qsTr("Max")
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
            id: parameterComboBox
            enabled: root.target !== null
            popup: ColumnsPopup {
                comboBox: parameterComboBox
            }
            model: root.target ? root.target.parameters.map(parameter => parameter.label) : []
            currentIndex: root.target ? root.target.parameters.findIndex(parameter => parameter.key === root.parameter) : -1
            onActivated: index => root.targetChosen(root.target.owner, root.target.ownerId, root.target.parameters[index].key)
            Layout.fillWidth: true
            Layout.preferredWidth: grid.share
        }
        MinSpinBox {
            visible: root.showsRangeBeside
            Layout.preferredWidth: grid.rangeWidth
        }
        MaxSpinBox {
            visible: root.showsRangeBeside
            Layout.preferredWidth: grid.rangeWidth
        }
    }

    RowLayout {
        visible: root.isMidi && !root.rangeBesideTarget
        spacing: 10
        Layout.fillWidth: true
        Label {
            text: qsTr("Min")
        }
        MinSpinBox {}
        Label {
            text: qsTr("Max")
        }
        MaxSpinBox {}
        Item {
            Layout.fillWidth: true
        }
    }

    Label {
        // Off centre, the middle of the swing bends: worth saying, since the pad's centre and an
        // LFO's zero are where nothing is expected to move
        visible: root.isMidi && root.pitchBend && root.midiMin + root.midiMax !== 0
        text: qsTr("The middle of the swing bends by %1 %").arg((root.midiMin + root.midiMax) / 2)
        color: Theme.warning
        font.pixelSize: Theme.bodyPixelSize
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
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
        text: root.isMidi ? qsTr("Also sent by another modulator: the last value sent wins") : qsTr("Also moved by another modulator: their movements add up")
        color: Theme.warning
        font.pixelSize: Theme.bodyPixelSize
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
