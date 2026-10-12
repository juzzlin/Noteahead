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
import QtQuick.Layouts 1.15
import Noteahead 1.0

//! The octave a device plays in, last in its Global section. Shared by every device that has one,
//! so the range and the readout are the same everywhere.
Knob {
    id: root
    //! The device's controller: anything with the octave property DeviceController gives it.
    property var controller
    label: qsTr("Octave")
    // Whole octaves only: read out as a count, and snapped to one on the way to the device
    mapping: "integer"
    mapMin: -2
    mapMax: 2
    from: -2
    to: 2
    stepSize: 1
    suffix: ""
    value: root.controller ? root.controller.octave : 0
    onMoved: v => root.controller.octave = Math.round(v)
    Layout.fillWidth: true
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Plays every note this many octaves up or down. Kept with the project, not in presets.")
}
