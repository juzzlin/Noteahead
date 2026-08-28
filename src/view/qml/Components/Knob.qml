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
    HoverHandler {
        id: knobHoverHandler
    }
    property string label: ""
    property real value: 0
    property real from: 0
    property real to: Constants.uiInternalScaling
    property string suffix: "%"
    property string mapping: "linear"
    property real mapMin: from
    property real mapMax: to
    property bool isInteger: true
    property real sliderFrom: from
    property real sliderTo: to
    property alias stepSize: slider.stepSize
    // Wheel steps are 1 % of the range, which is coarser than a pixel of drag on a narrow knob.
    // Ctrl buys the precision back, Shift trades it away for speed.
    //! Whether the pointer is over the knob, so a caller can hang a ToolTip on it. The root is a
    //! layout rather than a Control, so there is no hovered property to inherit.
    property alias hovered: knobHoverHandler.hovered
    property real fineWheelFactor: 0.1
    property real coarseWheelFactor: 5
    readonly property string displayValue: knobController.format((knobRoot.value - knobRoot.from) / (knobRoot.to - knobRoot.from), knobRoot.mapping, knobRoot.suffix, knobRoot.mapMin, knobRoot.mapMax)
    signal moved(real val)

    // Dragging a short knob cannot land on an exact value, so the readout doubles as an input field
    function commitTypedValue(text) {
        const position = knobController.parse(text, knobRoot.mapping, knobRoot.suffix, knobRoot.mapMin, knobRoot.mapMax);
        if (position !== undefined) {
            knobRoot.moved(knobRoot.from + position * (knobRoot.to - knobRoot.from));
        }
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    spacing: 2
    Label {
        id: valueLabel
        text: `${knobRoot.label} (${knobRoot.displayValue})`
        font.pixelSize: 11
        color: themeService.accentColor
        Layout.alignment: Qt.AlignHCenter

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.IBeamCursor
            onDoubleClicked: valueEditor.open()
            ToolTip.visible: containsMouse && !valueEditor.visible
            ToolTip.delay: 1000
            ToolTip.text: qsTr("Double-click to type an exact value")
        }

        Popup {
            id: valueEditor
            x: (valueLabel.width - width) / 2
            y: 0
            width: Math.max(knobRoot.width, 140)
            padding: 2
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            Universal.theme: Universal.Dark
            Universal.accent: themeService.accentColor

            onOpened: {
                valueField.text = knobRoot.displayValue;
                valueField.selectAll();
                valueField.forceActiveFocus();
            }

            contentItem: TextField {
                id: valueField
                horizontalAlignment: Text.AlignHCenter
                selectByMouse: true
                // Accepting the text as it was offered means nothing moved, which matters for the
                // readouts that show two numbers: re-reading one of them would nudge the other
                onAccepted: {
                    if (text !== knobRoot.displayValue) {
                        knobRoot.commitTypedValue(text);
                    }
                    valueEditor.close();
                }
            }
        }
    }
    Slider {
        id: slider
        from: knobRoot.sliderFrom
        to: knobRoot.sliderTo
        // Arrow keys step by stepSize, and Qt's fallback of 0.1 is a tenth of a decibel on a knob
        // that reads out in decibels but a ten-thousandth of the travel on one that runs 0..1000.
        // A share of the range steps the same everywhere. Dragging is unaffected: snapMode is
        // NoSnap, so stepSize only reaches the keyboard.
        stepSize: (knobRoot.sliderTo - knobRoot.sliderFrom) / 1000
        value: knobRoot.value
        Layout.fillWidth: true
        
        onMoved: {
            knobRoot.moved(value);
        }

        Binding {
            target: slider
            property: "value"
            value: knobRoot.value
            when: !slider.pressed
        }

        WheelHandler {
            onWheel: (wheel) => {
                const factor = (wheel.modifiers & Qt.ControlModifier) ? knobRoot.fineWheelFactor : ((wheel.modifiers & Qt.ShiftModifier) ? knobRoot.coarseWheelFactor : 1);
                const delta = (wheel.angleDelta.y / 12000.0) * (slider.to - slider.from) * factor;
                const nextV = Math.max(slider.from, Math.min(slider.to, slider.value + delta));
                knobRoot.moved(nextV);
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
