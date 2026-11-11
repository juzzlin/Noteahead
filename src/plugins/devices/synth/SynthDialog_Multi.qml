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

ColumnLayout {
    id: root
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    readonly property bool vpm: synthController.multiEngine === 1

    WaveShapeHeading {
        id: heading
        text: qsTr("Multi Engine")
        // Empty while the engine plays noise, which has no cycle to draw
        samples: {
            synthController.oscillatorPreviewRevision;
            return synthController.multiPreview(heading.previewPoints);
        }
        dimmed: synthController.multiLevel === 0
        Layout.fillWidth: true
        Layout.topMargin: 10
    }

    // One to a row: side by side at a module's width, the type's name is cut short
    ColumnLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: synthController.multiEngineNames
            currentIndex: synthController.multiEngine
            onActivated: i => synthController.multiEngine = i
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Noise, or VPM: a carrier whose phase a modulator moves, after the Minilogue XD's.")
            Layout.fillWidth: true
        }
        // Each engine keeps its own type, as on the XD
        ComboBox {
            model: root.vpm ? synthController.vpmTypeNames : synthController.multiTypeNames
            currentIndex: root.vpm ? synthController.vpmType : synthController.multiType
            onActivated: i => {
                if (root.vpm) {
                    synthController.vpmType = i;
                } else {
                    synthController.multiType = i;
                }
            }
            Layout.fillWidth: true
        }
    }

    RowLayout {
        visible: root.vpm
        spacing: 10
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Ratio")
            }
            ComboBox {
                model: synthController.vpmRatioOffsetNames
                currentIndex: synthController.vpmRatioOffset
                onActivated: i => synthController.vpmRatioOffset = i
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Multiplies the type's own modulator ratio, adding overtones. 1:1 leaves the type as it is.")
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Octave")
            }
            ComboBox {
                model: synthController.multiOctaveNames
                currentIndex: synthController.multiOctave
                onActivated: i => synthController.multiOctave = i
                ToolTip.visible: hovered
                ToolTip.text: qsTr("The octave VPM plays in. 8' plays the note.")
                Layout.fillWidth: true
            }
        }
    }

    Knob {
        label: qsTr("Shape")
        value: synthController.multiShape
        onMoved: v => synthController.multiShape = v
        ToolTip.visible: hovered && root.vpm
        ToolTip.text: qsTr("How deeply the modulator moves the carrier's phase.")
        Layout.fillWidth: true
    }
    Knob {
        visible: !root.vpm
        label: qsTr("Key Track")
        value: synthController.multiKeyTrack
        onMoved: v => synthController.multiKeyTrack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.multiLevel
        onMoved: v => synthController.multiLevel = v
        Layout.fillWidth: true
    }
}
