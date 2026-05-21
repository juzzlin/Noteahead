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
    property real sliderFrom: {
        if (mapping === "linear") return from;
        if (mapping === "cubicCentered") return -1.0;
        return 0.0;
    }
    property real sliderTo: {
        if (mapping === "linear") return to;
        if (mapping === "cubicCentered") return 1.0;
        return 1.0;
    }
    property alias stepSize: slider.stepSize
    signal moved(real val)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        text: {
            let displayValue = "";
            if (knobRoot.mapping === "linear") {
                displayValue = knobController.valueToString(knobRoot.value, knobRoot.suffix, knobRoot.from, knobRoot.to);
            } else {
                let mappedValue = knobController.map(knobRoot.value / knobRoot.to, knobRoot.mapping, knobRoot.mapMin, knobRoot.mapMax);
                displayValue = knobController.format(mappedValue, knobRoot.mapping, knobRoot.suffix, knobRoot.mapMin, knobRoot.mapMax);
            }
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
            let val = value;
            if (knobRoot.mapping === "linear") {
                knobRoot.moved(val);
            } else if (knobRoot.mapping === "cubicCentered") {
                // slider -1..1 maps to model 0..1000 via internal 0..1
                knobRoot.moved((val * 0.5 + 0.5) * knobRoot.to);
            } else {
                // slider 0..1 maps to model 0..1000
                knobRoot.moved(val * knobRoot.to);
            }
        }

        Binding {
            target: slider
            property: "value"
            value: {
                if (knobRoot.mapping === "linear") return knobRoot.value;
                if (knobRoot.mapping === "cubicCentered") {
                    return (knobRoot.value / knobRoot.to) * 2.0 - 1.0;
                }
                return knobRoot.value / knobRoot.to;
            }
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const delta = wheel.angleDelta.y > 0 ? 0.05 : -0.05;
                if (knobRoot.mapping === "linear") {
                    const range = knobRoot.to - knobRoot.from;
                    const linearDelta = delta * range;
                    knobRoot.moved(Math.max(knobRoot.from, Math.min(knobRoot.to, knobRoot.value + linearDelta)));
                } else {
                    const nextV = Math.max(slider.from, Math.min(slider.to, slider.value + delta));
                    if (knobRoot.mapping === "cubicCentered") {
                         knobRoot.moved((nextV * 0.5 + 0.5) * knobRoot.to);
                    } else {
                         knobRoot.moved(nextV * knobRoot.to);
                    }
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
