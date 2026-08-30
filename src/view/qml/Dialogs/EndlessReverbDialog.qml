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
    title: "<strong>" + qsTr("Endless Reverb (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 640
    height: 520

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: dialogScrollView.availableWidth
            spacing: 16

            GridLayout {
                columns: 4
                columnSpacing: 24
                rowSpacing: 20
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Size")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessSizeKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessSizeKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Feedback")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessFeedbackKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessFeedbackKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Pre-Delay")
                    suffix: "ms"
                    from: 0
                    to: 500
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessPreDelayKey()) * 500;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessPreDelayKey(), v / 500)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Damping")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessDampingKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessDampingKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mod Depth")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessModDepthKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessModDepthKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mod Rate")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessModRateKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessModRateKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Width")
                    mapping: "value"
                    from: 0
                    to: 200
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessWidthKey()) * 200;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessWidthKey(), v / 200)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Mix")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessMixKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessMixKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                FilterKnob {
                    label: qsTr("LPF")
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessLpfCutoffKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessLpfCutoffKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                FilterKnob {
                    label: qsTr("HPF")
                    isHpf: true
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessHpfCutoffKey()) * Constants.uiInternalScaling;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessHpfCutoffKey(), v / Constants.uiInternalScaling)
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    spacing: 10
                    Label {
                        text: qsTr("Freeze")
                        font.bold: true
                    }
                    Switch {
                        checked: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.endlessFreezeKey()) > 0.5;
                        }
                        onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.endlessFreezeKey(), checked ? 1 : 0)
                    }
                    Label {
                        text: qsTr("Sustain the tail infinitely")
                        color: "#aaa"
                        font.italic: true
                        font.pointSize: 10
                        Layout.fillWidth: true
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
