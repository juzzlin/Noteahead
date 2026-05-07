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
            let panVal = 0;
            if (knobRoot.from === 0 && knobRoot.to === Constants.uiInternalScaling) {
                // 0..1000 case (Synth Master Pan)
                panVal = (knobRoot.value / Constants.uiInternalScaling) * 200.0 - 100.0;
            } else if (knobRoot.from === -Constants.uiInternalScaling && knobRoot.to === Constants.uiInternalScaling) {
                // -1000..1000 case (Sampler Pad Pan)
                panVal = (knobRoot.value / Constants.uiInternalScaling) * 100.0;
            } else {
                // Fallback for other ranges, assuming center is in the middle
                let range = knobRoot.to - knobRoot.from;
                if (range === 0) return `${knobRoot.label} (Center)`;
                panVal = ((knobRoot.value - knobRoot.from) / range) * 200.0 - 100.0;
            }

            if (Math.abs(panVal) < 0.05) {
                return `${knobRoot.label} (${qsTr("Center")})`;
            }

            let displayValue = Math.abs(panVal).toFixed(1);
            let side = panVal < 0 ? qsTr(" L") : qsTr(" R");
            let sign = panVal > 0 ? "+" : "-";
            return `${knobRoot.label} (${sign}${displayValue}%${side})`;
        }
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

        WheelHandler {
            onWheel: (wheel) => {
                if (wheel.angleDelta.y > 0) {
                    slider.increase();
                } else {
                    slider.decrease();
                }
            }
        }
    }
}
