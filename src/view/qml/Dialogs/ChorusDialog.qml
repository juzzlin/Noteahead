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
    title: "<strong>" + qsTr("Chorus Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 600
    height: 450

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

        GridLayout {
            columns: 4
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true

            Knob {
                label: qsTr("Rate")
                suffix: "Hz"
                mapping: "exponential"
                mapMin: 0.1
                mapMax: 10.0
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusRateKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusRateKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Depth")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusDepthKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusDepthKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Delay")
                suffix: "ms"
                mapping: "linear"
                mapMin: 1.0
                mapMax: 50.0
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusDelayKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusDelayKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Width")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusWidthKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusWidthKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            FilterKnob {
                label: qsTr("LPF")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusLpfKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusLpfKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            FilterKnob {
                label: qsTr("HPF")
                isHpf: true
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusHpfKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusHpfKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Mix")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.chorusMixKey()) * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.chorusMixKey(), v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }
        }

        Item { Layout.fillHeight: true }
    }
}
