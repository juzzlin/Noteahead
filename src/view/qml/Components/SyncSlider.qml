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
    signal moved(real val)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        text: {
            const index = slider.findBestIndex(knobRoot.value / Constants.uiInternalScaling);
            return `${knobRoot.label} (${Constants.syncLabels[index]})`;
        }
        font.pixelSize: 11
        color: themeService.accentColor
        Layout.alignment: Qt.AlignHCenter
    }

    Slider {
        id: slider
        from: 0
        to: Constants.syncDivisions.length - 1
        stepSize: 1
        Layout.fillWidth: true
        padding: 0

        function findBestIndex(val) {
            let bestIdx = 0;
            let minDiff = 10.0;
            for (let i = 0; i < Constants.syncDivisions.length; ++i) {
                let diff = Math.abs(Constants.syncDivisions[i] - val);
                if (diff < minDiff) {
                    minDiff = diff;
                    bestIdx = i;
                }
            }
            return bestIdx;
        }

        onMoved: {
            knobRoot.moved(Constants.syncDivisions[Math.round(value)] * Constants.uiInternalScaling);
        }

        Binding {
            target: slider
            property: "value"
            value: slider.findBestIndex(knobRoot.value / Constants.uiInternalScaling)
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const delta = wheel.angleDelta.y > 0 ? 1 : -1;
                const newIndex = Math.max(0, Math.min(Constants.syncDivisions.length - 1, slider.value + delta));
                knobRoot.moved(Constants.syncDivisions[newIndex] * Constants.uiInternalScaling);
            }
        }
    }
}
