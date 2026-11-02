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

// The six parameters the Minilogue XD keeps for each VPM type, each a step either way of what the
// type does on its own: at the middle the type is as designed, at the bottom the setting is gone.
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: "VPM"
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    component OffsetKnob: Knob {
        mapMin: -100
        mapMax: 100
        Layout.fillWidth: true
    }

    OffsetKnob {
        label: qsTr("Feedback")
        value: synthController.vpmFeedback
        onMoved: v => synthController.vpmFeedback = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the modulator feeds back into itself, around the type's own amount. More is rougher, toward a saw.")
    }
    OffsetKnob {
        label: qsTr("Noise Depth")
        value: synthController.vpmNoiseDepth
        onMoved: v => synthController.vpmNoiseDepth = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far noise moves the carrier's phase, around the type's own amount.")
    }
    OffsetKnob {
        label: qsTr("Shape Mod Int")
        value: synthController.vpmShapeModInt
        onMoved: v => synthController.vpmShapeModInt = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the internal envelope deepens the modulation, around the type's own amount. All the way down switches the envelope off.")
    }
    OffsetKnob {
        label: qsTr("Mod Attack")
        value: synthController.vpmModAttack
        onMoved: v => synthController.vpmModAttack = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long the internal envelope takes to rise, around the type's own time.")
    }
    OffsetKnob {
        label: qsTr("Mod Decay")
        value: synthController.vpmModDecay
        onMoved: v => synthController.vpmModDecay = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How long the internal envelope takes to die away, around the type's own time.")
    }
    OffsetKnob {
        label: qsTr("Mod Key Track")
        value: synthController.vpmModKeyTrack
        onMoved: v => synthController.vpmModKeyTrack = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How much the modulation backs off on higher notes, around the type's own amount.")
    }
}
