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
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("String")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Decay")
        value: pianoSynthAcousticController.decay
        onMoved: v => pianoSynthAcousticController.decay = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the strings ring while the key is held.")
    }
    Knob {
        label: qsTr("Brightness")
        value: pianoSynthAcousticController.brightness
        onMoved: v => pianoSynthAcousticController.brightness = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the upper partials ring against the lower ones.")
    }
    Knob {
        label: qsTr("Inharmonicity")
        value: pianoSynthAcousticController.inharmonicity
        onMoved: v => pianoSynthAcousticController.inharmonicity = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How stiff the strings are: stiffer strings stretch their upper partials sharp, which is much of what makes a piano sound like one.")
    }
    Knob {
        label: qsTr("Unison Detune")
        value: pianoSynthAcousticController.detune
        onMoved: v => pianoSynthAcousticController.detune = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How far apart the two or three strings of a key are tuned: a concert grand at the bottom, a bar-room upright at the top.")
    }
    Knob {
        label: qsTr("Stretch")
        value: pianoSynthAcousticController.stretch
        onMoved: v => pianoSynthAcousticController.stretch = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How much of the octave stretch a piano tuner gives a grand: flat bass and sharp treble. At zero the piano plays in equal temperament.")
    }
}
