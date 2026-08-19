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
    title: "<strong>" + qsTr("Early Reflections (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
                text: qsTr("The reflections that arrive before a reverb becomes a wash. This is the axis a tail cannot buy: how far away something is, not how large the room around it sounds.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 4
                columnSpacing: 24
                rowSpacing: 16
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Size")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsSizeKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsSizeKey(), v / 100)
                }

                Knob {
                    label: qsTr("Pre-Delay")
                    suffix: "ms"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsPreDelayKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsPreDelayKey(), v / 100)
                }

                Knob {
                    label: qsTr("Damping")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsDampingKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsDampingKey(), v / 100)
                }

                Knob {
                    label: qsTr("Diffusion")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsDiffusionKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsDiffusionKey(), v / 100)
                }

                Knob {
                    label: qsTr("Width")
                    suffix: "%"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 200
                    from: 0
                    to: 200
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsWidthKey()) * 200;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsWidthKey(), v / 200)
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
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsLowCutKey()) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsLowCutKey(), v / 1000)
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsMixKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsMixKey(), v / 100)
                }

                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.earlyReflectionsSoloKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.earlyReflectionsSoloKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only the reflections, so the room can be heard without the source that caused it")
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
