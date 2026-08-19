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

EffectDialog {
    id: root
    title: "<strong>" + qsTr("Dimension (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: Math.max(implicitWidth, scrollView.availableWidth)
            spacing: 16

            Label {
                text: qsTr("Builds width out of the centre of the mix. What it adds appears only in the side signal, so the mono sum is exactly what came in.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Detune")
                    suffix: "c"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 25
                    from: 0
                    to: 250
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.dimensionDetuneKey()) * 250;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.dimensionDetuneKey(), v / 250)
                }

                Knob {
                    label: qsTr("Amount")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.dimensionAmountKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.dimensionAmountKey(), v / 100)
                }

                Knob {
                    label: qsTr("Low Cut")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 20
                    mapMax: 500
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.dimensionLowCutKey()) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.dimensionLowCutKey(), v / 1000)
                }

                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.dimensionSoloKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.dimensionSoloKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only the side signal this builds, so it can be heard on its own")
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
