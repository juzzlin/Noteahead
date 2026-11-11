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
        text: qsTr("Analog")
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("Drift")
        value: synthDController.drift
        onMoved: v => synthDController.drift = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the three VCOs slowly wander out of tune together")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Instability")
        value: synthDController.instability
        onMoved: v => synthDController.instability = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far each VCO wanders on its own, so that they beat slowly against each other")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Tolerance")
        value: synthDController.tolerance
        onMoved: v => synthDController.tolerance = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the VCOs are out of tune with each other, and the filter off its knob, for good")
        Layout.fillWidth: true
    }
}
