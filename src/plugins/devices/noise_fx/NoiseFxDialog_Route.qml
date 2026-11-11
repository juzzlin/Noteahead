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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

//! Where a sweep or the LFO goes: two destinations, each with a depth either way.
ColumnLayout {
    id: root

    //! The ordinals of the two destinations and their depth knobs.
    property int target1: 0
    property int target2: 0
    property int depth1: 0
    property int depth2: 0
    //! The LFO cannot move its own rate or depth, so it offers neither.
    property bool isLfo: false

    signal target1Activated(int target)
    signal target2Activated(int target)
    signal depth1Moved(int depth)
    signal depth2Moved(int depth)

    // In NoiseFxDevice::Destination order, which the project file stores
    readonly property var destinationNames: {
        languageService.activeLanguage;
        return [qsTr("None"), qsTr("Cutoff"), qsTr("Resonance"), qsTr("Colour"), qsTr("S&H Rate"), qsTr("Level"), qsTr("Pan"), qsTr("Pitch"), qsTr("Blend"), qsTr("LFO Rate"), qsTr("LFO Depth"), qsTr("Gate Rate")];
    }
    // The LFO's own rate and depth, ordinals 9 and 10, are left out of its list
    readonly property var offeredNames: isLfo ? destinationNames.filter((name, index) => index !== 9 && index !== 10) : destinationNames

    function toOrdinal(index) {
        return isLfo && index >= 9 ? index + 2 : index;
    }
    function toIndex(ordinal) {
        return isLfo && ordinal >= 11 ? ordinal - 2 : ordinal;
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true

    Repeater {
        model: 2
        ColumnLayout {
            id: destination
            required property int index
            readonly property int target: index === 0 ? root.target1 : root.target2
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true
                Layout.bottomMargin: Constants.dropDownBottomMargin
                Label {
                    text: qsTr("Target") + " " + (destination.index + 1)
                }
                ComboBox {
                    model: root.offeredNames
                    currentIndex: root.toIndex(destination.target)
                    onActivated: i => destination.index === 0 ? root.target1Activated(root.toOrdinal(i)) : root.target2Activated(root.toOrdinal(i))
                    Layout.fillWidth: true
                }
            }
            Knob {
                label: qsTr("Depth") + " " + (destination.index + 1)
                mapping: "bipolar"
                mapMin: -100
                mapMax: 100
                enabled: destination.target !== 0
                value: destination.index === 0 ? root.depth1 : root.depth2
                onMoved: v => destination.index === 0 ? root.depth1Moved(v) : root.depth2Moved(v)
                Layout.fillWidth: true
            }
        }
    }
}
