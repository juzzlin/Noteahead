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
    property real to: 1000
    signal moved(real val)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        text: `${knobRoot.label} (${knobController.panToString(knobRoot.value, knobRoot.from, knobRoot.to)})`
        font.pixelSize: 11
        color: themeService.accentColor
        Layout.alignment: Qt.AlignHCenter
    }

    Slider {
        id: slider
        from: -1.0
        to: 1.0
        stepSize: 0
        Layout.fillWidth: true

        function updateValue(v: double): void {
            knobRoot.moved(knobController.mapPan(v, knobRoot.from, knobRoot.to));
        }

        onMoved: updateValue(value)

        Binding {
            target: slider
            property: "value"
            value: knobController.unmapPan(knobRoot.value, knobRoot.from, knobRoot.to)
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const currentV = knobController.unmapPan(knobRoot.value, knobRoot.from, knobRoot.to);
                const delta = wheel.angleDelta.y > 0 ? 0.05 : -0.05;
                slider.updateValue(Math.max(-1, Math.min(1, currentV + delta)));
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onPressed: mouse => {
                if (mouse.button === Qt.RightButton) {
                    const center = (knobRoot.from + knobRoot.to) / 2.0;
                    knobRoot.moved(center);
                } else {
                    mouse.accepted = false;
                }
            }
        }
    }
}
