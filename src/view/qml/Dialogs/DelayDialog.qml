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

    title: "<strong>" + qsTr("Delay Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 500
    height: 500

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
            columns: 2
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 5
                Label {
                    text: qsTr("Type")
                    font.bold: true
                }
                ComboBox {
                    model: ["Stereo", "Mono", "PingPong", "Tape"]
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, "delayType") * 3);
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, "delayType", index / 3.0)
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: 5
                Label {
                    text: qsTr("Sync")
                    font.bold: true
                }
                CheckBox {
                    text: qsTr("Enable BPM Sync")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "delaySync") > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, "delaySync", checked ? 1.0 : 0.0)
                    Layout.fillWidth: true
                }
            }

            StackLayout {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delaySync") > 0.5 ? 0 : 1;
                }

                SyncSlider {
                    label: qsTr("Time (Sync)")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "delaySyncDivision") * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delaySyncDivision", v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Time")
                    suffix: "ms"
                    mapping: "cubic"
                    mapMin: 1
                    mapMax: 10000
                    to: 10000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, "delayTime") * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayTime", v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }
            }

            Knob {
                label: qsTr("Feedback")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delayFeedback") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayFeedback", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Depth")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delayDepth") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayDepth", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            FilterKnob {
                label: qsTr("LPF")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delayFeedbackLpf") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayFeedbackLpf", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            FilterKnob {
                label: qsTr("HPF")
                isHpf: true
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delayFeedbackHpf") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayFeedbackHpf", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Mix")
                Layout.columnSpan: 2
                Layout.fillWidth: true
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "delayMix") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "delayMix", v / Constants.uiInternalScaling)
            }
        }

        Item { Layout.fillHeight: true }
    }
}
