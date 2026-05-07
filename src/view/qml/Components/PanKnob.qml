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
        text: {
            const center = (knobRoot.from + knobRoot.to) / 2.0;
            const range = (knobRoot.to - knobRoot.from) / 2.0;
            const panVal = range !== 0 ? ((knobRoot.value - center) / range) * 100.0 : 0;

            if (Math.abs(panVal) < 0.05) {
                return `${knobRoot.label} (${qsTr("Center")})`;
            }

            const displayValue = Math.abs(panVal).toFixed(1);
            const side = panVal < 0 ? qsTr(" L") : qsTr(" R");
            const sign = panVal > 0 ? "+" : "-";
            return `${knobRoot.label} (${sign}${displayValue}%${side})`;
        }
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

        function updateValue(v) {
            const mapped = Math.sign(v) * Math.pow(Math.abs(v), 3.0);
            const center = (knobRoot.from + knobRoot.to) / 2.0;
            const range = (knobRoot.to - knobRoot.from) / 2.0;
            let outVal = mapped * range + center;

            // Snap to center (within 1% of total range)
            if (Math.abs(outVal - center) < (range * 0.01)) {
                 outVal = center;
            }

            knobRoot.moved(outVal);
        }

        onMoved: updateValue(value)

        Binding {
            target: slider
            property: "value"
            value: {
                const center = (knobRoot.from + knobRoot.to) / 2.0;
                const range = (knobRoot.to - knobRoot.from) / 2.0;
                if (range === 0) return 0;
                const norm = Math.max(-1, Math.min(1, (knobRoot.value - center) / range));
                return Math.sign(norm) * Math.pow(Math.abs(norm), 1.0/3.0);
            }
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const delta = wheel.angleDelta.y > 0 ? 0.05 : -0.05;
                const newValue = Math.max(-1, Math.min(1, slider.value + delta));
                slider.value = newValue;
                slider.updateValue(newValue);
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
