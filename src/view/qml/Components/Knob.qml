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

ColumnLayout {
    id: knobRoot
    property string label: ""
    property real value: 0
    property real from: 0
    property real to: 100
    property string suffix: "%"
    property alias stepSize: slider.stepSize
    signal moved(real val)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        text: knobRoot.label + " (" + Math.round(knobRoot.value) + knobRoot.suffix + ")"
        font.pixelSize: 11
        color: themeService.accentColor
        Layout.alignment: Qt.AlignHCenter
    }
    Slider {
        id: slider
        from: knobRoot.from
        to: knobRoot.to
        value: knobRoot.value
        stepSize: 1
        Layout.fillWidth: true
        onMoved: () => knobRoot.moved(value)
    }
}
