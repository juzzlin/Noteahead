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
        text: qsTr("Hammer")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Hardness")
        value: pianoSynthAcousticController.hardness
        onMoved: v => pianoSynthAcousticController.hardness = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How hard the felt of the hammers is: soft and round, or hard and ringing. Playing harder hardens it too.")
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: pianoSynthAcousticController.velocitySensitivity
        onMoved: v => pianoSynthAcousticController.velocitySensitivity = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How much softer notes play softer. A soft note is darker whatever this is set to, as the hammer felt is less compressed.")
    }
    Knob {
        label: qsTr("Knock")
        value: pianoSynthAcousticController.knock
        onMoved: v => pianoSynthAcousticController.knock = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How much of the hammer is heard knocking the soundboard and the rim, the thump under every note.")
    }
}
