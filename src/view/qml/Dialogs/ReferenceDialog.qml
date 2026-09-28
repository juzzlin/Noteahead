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
    title: "<strong>" + qsTr("Reference (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
                text: qsTr("Plays the mix the way another system would: its band limits, its voicing, the room or box it rings in, and the compression it applies. Indicative rather than exact, and heard only: an export is never touched.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            EffectPresetRow {
                effectIndex: root.effectIndex
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Label {
                    text: qsTr("System")
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }

                ComboBox {
                    id: environmentCombo
                    // Read from the effect's own table, so this cannot name a system it does not have
                    model: effectRackController.referenceEnvironmentNames()
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.referenceEnvironmentKey()) * (count - 1));
                    }
                    onActivated: effectRackController.setParameterValue(root.effectIndex, effectRackController.referenceEnvironmentKey(), count > 1 ? currentIndex / (count - 1) : 0)
                    Universal.theme: Universal.Dark
                    implicitWidth: 160
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Amount")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.referenceAmountKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.referenceAmountKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far from this room towards that one. At zero the mix is heard as it is")
                }

                Knob {
                    label: qsTr("Room")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.referenceRoomKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.referenceRoomKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much of the space the system is heard in: the cabin, the club, the hall")
                }

                Knob {
                    label: qsTr("Dynamics")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.referenceDynamicsKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.referenceDynamicsKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much of that system's own compression and drive is applied")
                }

                Knob {
                    label: qsTr("Output")
                    suffix: " dB"
                    mapping: "value"
                    mapMin: -12
                    mapMax: 12
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.referenceGainKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.referenceGainKey(), v / 100)
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
