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
import "../Components"

Dialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("All-Pass Filter Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 450
    height: 300

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Knob {
            label: qsTr("Frequency")
            mapping: "logFrequency"
            mapMin: 20
            mapMax: 20000
            value: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, effectRackController.allPassFilterFrequencyKey()) * Constants.uiInternalScaling;
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.allPassFilterFrequencyKey(), v / Constants.uiInternalScaling)
            Layout.fillWidth: true

            HoverHandler { id: freqHover }
            ToolTip {
                visible: freqHover.hovered
                delay: Constants.toolTipDelay
                text: qsTr("Center frequency of the phase rotation. Does not filter any frequencies — only shifts phase relationships around this point. Set near the fundamental of the source (e.g. 60–100 Hz for kick drum).")
            }
        }

        Knob {
            label: qsTr("Q")
            suffix: ""
            from: 0.1
            to: 10.0
            isInteger: false
            value: {
                effectRackController.revision;
                return 0.1 + effectRackController.parameterValue(root.effectIndex, effectRackController.allPassFilterQKey()) * 9.9;
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.allPassFilterQKey(), (v - 0.1) / 9.9)
            Layout.fillWidth: true

            HoverHandler { id: qHover }
            ToolTip {
                visible: qHover.hovered
                delay: Constants.toolTipDelay
                text: qsTr("Bandwidth of the phase rotation. Lower Q = broader effect across more frequencies. Higher Q = more focused rotation around the center frequency. 0.707 is the Butterworth (maximally flat) response.")
            }
        }

        Knob {
            label: qsTr("Stages")
            suffix: ""
            from: 1
            to: 4
            stepSize: 1
            value: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, effectRackController.allPassFilterStagesKey());
            }
            onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.allPassFilterStagesKey(), Math.round(v))
            Layout.fillWidth: true

            HoverHandler { id: stagesHover }
            ToolTip {
                visible: stagesHover.hovered
                delay: Constants.toolTipDelay
                text: qsTr("Number of cascaded all-pass filter stages. More stages = greater total phase rotation = stronger tightening effect. Start with 1–2 and increase to taste.")
            }
        }
    }
}
