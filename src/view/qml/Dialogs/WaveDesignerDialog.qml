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
    title: "<strong>" + qsTr("Wave Designer Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 600
    height: 420

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
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        RowLayout {
            width: scrollView.availableWidth
            spacing: 20

            GridLayout {
                columns: 2
                columnSpacing: 30
                rowSpacing: 20
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                Knob {
                    Layout.row: 0
                    Layout.column: 0
                    label: qsTr("Attack")
                    suffix: "%"
                    mapping: "bipolar"
                    mapMin: -100
                    mapMax: 100
                    from: -100
                    to: 100
                    value: {
                        effectRackController.revision;
                        return (effectRackController.parameterValue(root.effectIndex, effectRackController.waveDesignerAttackKey()) - 0.5) * 200;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.waveDesignerAttackKey(), v / 200 + 0.5)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 0
                    Layout.column: 1
                    label: qsTr("Sustain")
                    suffix: "%"
                    mapping: "bipolar"
                    mapMin: -100
                    mapMax: 100
                    from: -100
                    to: 100
                    value: {
                        effectRackController.revision;
                        return (effectRackController.parameterValue(root.effectIndex, effectRackController.waveDesignerSustainKey()) - 0.5) * 200;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.waveDesignerSustainKey(), v / 200 + 0.5)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 1
                    Layout.column: 0
                    label: qsTr("Gain")
                    suffix: "dB"
                    from: -24
                    to: 24
                    value: {
                        effectRackController.revision;
                        return (effectRackController.parameterValue(root.effectIndex, effectRackController.waveDesignerGainKey()) - 0.5) * 48;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.waveDesignerGainKey(), v / 48 + 0.5)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 1
                    Layout.column: 1
                    label: qsTr("Mix")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.waveDesignerMixKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.waveDesignerMixKey(), v / 100)
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: 10
                Layout.alignment: Qt.AlignTop

                Label {
                    text: qsTr("Shaping")
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // Centre-zero meter: the shaper both lifts and tames, so the bar grows upwards for
                // gain and downwards for attenuation.
                Rectangle {
                    width: 30
                    height: 200
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    Rectangle {
                        width: parent.width - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: Math.min((parent.height - 4) / 2, (Math.abs(root.currentShapingDb) / 15.0) * (parent.height - 4) / 2)
                        y: root.currentShapingDb >= 0 ? parent.height / 2 - height : parent.height / 2
                        color: themeService.accentColor
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        y: parent.height / 2
                        color: "#666"
                    }
                }

                // Fixed width, because the reading is what the rest of the dialog is laid out
                // against: letting it resize as the value crosses ten or zero, or gains a sign,
                // shifts everything beside it on every meter tick.
                Label {
                    text: (root.currentShapingDb >= 0 ? "+" : "") + root.currentShapingDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 90
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    property real currentShapingDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentShapingDb = effectRackController.waveDesignerShapingDb(root.effectIndex);
        }
    }
}
