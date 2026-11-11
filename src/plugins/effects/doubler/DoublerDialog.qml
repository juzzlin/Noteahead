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
    title: "<strong>" + qsTr("Doubler (Slot %1)").arg(effectIndex + 1) + "</strong>"
    width: parent ? parent.width * Constants.defaultDialogScale : 700
    height: parent ? parent.height * Constants.defaultDialogScale : 560
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            Label {
                text: qsTr("Doubles the part with a copy of itself a few milliseconds late. Mono Safe puts the copy in the side signal only, so the mono sum is exactly what came in. Haas puts it on the right channel: wider and more obvious, but it combs in mono.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20
                Label {
                    text: qsTr("Mode")
                    font.bold: true
                }
                ComboBox {
                    model: [qsTr("Mono Safe"), qsTr("Haas")]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.mode);
                    }
                    onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.mode, i)
                    Layout.preferredWidth: 160
                }
            }

            GridLayout {
                columns: 2
                columnSpacing: 24
                rowSpacing: 16
                Layout.fillWidth: true

                Knob {
                    label: qsTr("Time")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 2
                    mapMax: 50
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.delayTime) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.delayTime, v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Amount")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.amount) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.amount, v / 100)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Low Cut")
                    suffix: "Hz"
                    mapping: "logFrequencyHighPass"
                    mapMin: 50
                    mapMax: 5000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.hpfCutoff) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.hpfCutoff, v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Drift")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.depth) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.depth, v / 100)
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Let the delay wander slowly, as a tape double does")
                }

                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, NahdXml.solo) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.solo, checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only what the copy adds, so it can be heard on its own")
                }
            }
        }
    }
}
