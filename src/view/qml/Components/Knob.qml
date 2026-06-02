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
    property real to: Constants.uiInternalScaling
    property string suffix: "%"
    property string mapping: "linear"
    property real mapMin: 0
    property real mapMax: 1
    property bool isInteger: true
    property real sliderFrom: {
        if (mapping === "linear") return from;
        if (mapping === "cubicCentered" || mapping === "pan" || mapping === "intensity") return -1.0;
        return 0.0;
    }
    property real sliderTo: {
        if (mapping === "linear") return to;
        if (mapping === "cubicCentered" || mapping === "pan" || mapping === "intensity") return 1.0;
        return 1.0;
    }
    property alias stepSize: slider.stepSize
    signal moved(real val)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        text: {
            let modelNorm = knobRoot.value / knobRoot.to;
            let displayValue = knobController.format(modelNorm, knobRoot.mapping, knobRoot.suffix, knobRoot.mapMin, knobRoot.mapMax);
            return `${knobRoot.label} (${displayValue})`;
        }
        font.pixelSize: 11
        color: themeService.accentColor
        Layout.alignment: Qt.AlignHCenter
    }
    Slider {
        id: slider
        from: knobRoot.sliderFrom
        to: knobRoot.sliderTo
        stepSize: 0
        Layout.fillWidth: true
        
        onMoved: {
            if (knobRoot.mapping === "linear") {
                knobRoot.moved(value);
            } else {
                let norm = (value - from) / (to - from);
                let mapped = knobController.map(norm, knobRoot.mapping, knobRoot.mapMin, knobRoot.mapMax);
                knobRoot.moved(mapped * knobRoot.to);
            }
        }

        Binding {
            target: slider
            property: "value"
            value: {
                if (knobRoot.mapping === "linear") return knobRoot.value;
                let modelNorm = knobRoot.value / knobRoot.to;
                let unmapped = knobController.unmap(modelNorm, knobRoot.mapping, knobRoot.mapMin, knobRoot.mapMax);
                return unmapped * (sliderTo - sliderFrom) + sliderFrom;
            }
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const delta = wheel.angleDelta.y / 12000.0; // ~1% per standard notch
                const nextV = Math.max(slider.from, Math.min(slider.to, slider.value + delta));
                if (knobRoot.mapping === "linear") {
                    knobRoot.moved(nextV);
                } else {
                    let norm = (nextV - slider.from) / (slider.to - slider.from);
                    let mapped = knobController.map(norm, knobRoot.mapping, knobRoot.mapMin, knobRoot.mapMax);
                    knobRoot.moved(mapped * knobRoot.to);
                }
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
