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

//! One pad of the XY pad device: what each axis moves, how far, and how the pad comes back. The
//! dialog repeats this over xyPadController.pads.
ColumnLayout {
    id: root

    //! The XyPadPanelController this panel edits.
    required property var pad
    //! Everything a modulator can move, as propertyService.modulationTargets() lists it.
    required property var targets

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 6

    SectionHeading {
        text: qsTr("Pad %1").arg(root.pad.index + 1)
    }
    ModulationTargetPicker {
        label: qsTr("X target")
        targets: root.targets
        owner: root.pad.xTargetOwner
        ownerId: root.pad.xTargetOwnerId
        parameter: root.pad.xTargetParameter
        shared: root.pad.xTargetShared
        pitchBend: root.pad.xPitchBend
        midiMin: root.pad.xMidiMin
        midiMax: root.pad.xMidiMax
        onMidiMinChosen: value => root.pad.xMidiMin = value
        onMidiMaxChosen: value => root.pad.xMidiMax = value
        onTargetChosen: (owner, ownerId, parameter) => root.pad.setTarget(0, owner, ownerId, parameter)
        onOwnerChosen: (owner, ownerId, parameters) => root.pad.chooseTargetOwner(0, owner, ownerId, parameters)
        onTargetCleared: root.pad.clearTarget(0)
    }
    ModulationTargetPicker {
        label: qsTr("Y target")
        targets: root.targets
        owner: root.pad.yTargetOwner
        ownerId: root.pad.yTargetOwnerId
        parameter: root.pad.yTargetParameter
        shared: root.pad.yTargetShared
        pitchBend: root.pad.yPitchBend
        midiMin: root.pad.yMidiMin
        midiMax: root.pad.yMidiMax
        onMidiMinChosen: value => root.pad.yMidiMin = value
        onMidiMaxChosen: value => root.pad.yMidiMax = value
        onTargetChosen: (owner, ownerId, parameter) => root.pad.setTarget(1, owner, ownerId, parameter)
        onOwnerChosen: (owner, ownerId, parameters) => root.pad.chooseTargetOwner(1, owner, ownerId, parameters)
        onTargetCleared: root.pad.clearTarget(1)
    }
    GridLayout {
        columns: 3
        columnSpacing: 10
        Layout.fillWidth: true
        // An edge of the pad swings half the depth either way of the setting, so it reads as that swing
        Knob {
            label: qsTr("X Depth")
            // A controller goes between its Min and Max instead
            enabled: root.pad.xTargetOwner !== "midi"
            mapping: "swing"
            mapMax: 50
            value: root.pad.xDepth
            onMoved: value => root.pad.xDepth = value
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Knob {
            label: qsTr("Y Depth")
            // A controller goes between its Min and Max instead
            enabled: root.pad.yTargetOwner !== "midi"
            mapping: "swing"
            mapMax: 50
            value: root.pad.yDepth
            onMoved: value => root.pad.yDepth = value
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Knob {
            label: qsTr("Return")
            mapping: "cubic"
            mapMin: 0
            mapMax: 5000
            suffix: "ms"
            value: root.pad.returnTime
            onMoved: value => root.pad.returnTime = value
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How long the pad takes to glide back to the centre when let go. At 0 ms it stays where it was let go.")
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
    }
}
