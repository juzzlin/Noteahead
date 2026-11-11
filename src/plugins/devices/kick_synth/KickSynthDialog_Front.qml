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
        text: qsTr("Front Head & Mic")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Front Head")
        value: kickSynthController.frontHead
        onMoved: v => kickSynthController.frontHead = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Ported or removed at the bottom, solid at the top: the long boom and the beating of two heads.")
    }
    Knob {
        label: qsTr("Front Tune")
        mapping: "exponential"
        mapMin: 0.8
        mapMax: 3
        suffix: "x"
        value: kickSynthController.frontTune
        onMoved: v => kickSynthController.frontTune = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The pitch of the front head against the batter head.")
    }
    Knob {
        label: qsTr("Shell")
        value: kickSynthController.shell
        onMoved: v => kickSynthController.shell = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The knock of the shell, a few times above the pitch of the head.")
    }
    Knob {
        label: qsTr("Mic")
        value: kickSynthController.mic
        onMoved: v => kickSynthController.mic = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Inside the drum by the beater at the bottom, outside the front head at the top: attack against boom.")
    }
}
