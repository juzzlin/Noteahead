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
    property string label: qsTr("Target")

    signal targetChosen(string owner, string ownerId, string parameter)
    signal targetCleared

    readonly property int targetIndex: root.ownerId === "" ? -1 : Targets.findTargetIndex(root.targets, root.owner, root.ownerId, root.parameter)
    readonly property var target: root.targetIndex >= 0 ? root.targets[root.targetIndex] : null

    Layout.fillWidth: true
    spacing: 6

    GridLayout {
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
            model: [qsTr("None")].concat(root.targets.map(target => Targets.targetLabel(target)))
            // A target that is no longer in the project reads as nothing chosen rather than as
            // something else
            currentIndex: root.targetIndex + 1
            onActivated: index => {
                if (index === 0) {
                    root.targetCleared();
                } else {
                    const target = root.targets[index - 1];
                    root.targetChosen(target.owner, target.ownerId, target.parameters.length ? target.parameters[0].key : "");
                }
            }
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        ComboBox {
            enabled: root.target !== null
            model: root.target ? root.target.parameters.map(parameter => parameter.label) : []
            currentIndex: root.target ? root.target.parameters.findIndex(parameter => parameter.key === root.parameter) : -1
            onActivated: index => root.targetChosen(root.target.owner, root.target.ownerId, root.target.parameters[index].key)
            Layout.fillWidth: true
            Layout.preferredWidth: 1
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
        text: qsTr("Also moved by another modulator: their movements add up")
        color: Theme.warning
        font.pixelSize: Theme.bodyPixelSize
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
