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

// The 909's own three knobs, read 0 to 10 as on the panel
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Bass Drum")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Tune")
        mapping: "value"
        mapMin: 0
        mapMax: 10
        suffix: ""
        value: kick909Controller.tune
        onMoved: v => kick909Controller.tune = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the pitch takes to fall: a snap at 0, a slow sweep at 10. The drum settles at the same pitch whatever the setting.")
    }
    Knob {
        label: qsTr("Attack")
        mapping: "value"
        mapMin: 0
        mapMax: 10
        suffix: ""
        value: kick909Controller.attack
        onMoved: v => kick909Controller.attack = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The click at the start of the drum and the noise over it. The lower half is the 909's own; the top half goes beyond it, to a hard click.")
    }
    Knob {
        label: qsTr("Decay")
        mapping: "value"
        mapMin: 0
        mapMax: 10
        suffix: ""
        value: kick909Controller.decay
        onMoved: v => kick909Controller.decay = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the drum sounds: about a tenth of a second at 0, half a second at 10. Use the amp envelope for longer.")
    }
}
