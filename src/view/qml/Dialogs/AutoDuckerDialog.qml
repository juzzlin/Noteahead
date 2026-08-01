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

AnimatedDialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("Auto Ducker Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 650
    height: 550

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
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 30

            GridLayout {
                columns: 3
                columnSpacing: 30
                rowSpacing: 20
                Layout.fillWidth: true

                Knob {
                    Layout.row: 0
                    Layout.column: 0
                    label: qsTr("Threshold")
                    suffix: "dB"
                    from: -60
                    to: 0
                    value: {
                        effectRackController.revision;
                        return -60 + effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerThresholdKey()) * 60;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerThresholdKey(), (v + 60) / 60)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 0
                    Layout.column: 1
                    label: qsTr("Amount")
                    suffix: "dB"
                    from: -24
                    to: 24
                    value: {
                        effectRackController.revision;
                        return -24 + effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerAmountKey()) * 48;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerAmountKey(), (v + 24) / 48)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 0
                    Layout.column: 2
                    label: qsTr("Knee")
                    suffix: "dB"
                    from: 0
                    to: 24
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerKneeKey()) * 24;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerKneeKey(), v / 24)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 1
                    Layout.column: 0
                    label: qsTr("Attack")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 0.1
                    mapMax: 500
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerAttackKey()) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerAttackKey(), v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 1
                    Layout.column: 1
                    label: qsTr("Release")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 1.0
                    mapMax: 2000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerReleaseKey()) * 1000;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerReleaseKey(), v / 1000)
                    Layout.fillWidth: true
                }

                Knob {
                    Layout.row: 1
                    Layout.column: 2
                    label: qsTr("Hold")
                    suffix: "ms"
                    from: 0
                    to: 500
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerHoldKey()) * 500;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerHoldKey(), v / 500)
                    Layout.fillWidth: true
                }

                Label {
                    Layout.row: 2
                    Layout.column: 0
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    text: qsTr("Negative Amount ducks the signal, positive lifts it. Without a side chain source the effect listens to its own input.")
                    color: "#aaa"
                    font.italic: true
                    font.pointSize: 10
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.row: 3
                    Layout.column: 0
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    spacing: 10
                    Label {
                        text: qsTr("Side Chain Source:")
                        font.bold: true
                    }
                    ComboBox {
                        id: sideChainCombo
                        Layout.fillWidth: true
                        model: {
                            var items = [qsTr("None")];
                            for (var i = 0; i < deviceRackController.deviceCount; i++) {
                                items.push(qsTr("Device %1").arg(i + 1));
                            }
                            return items;
                        }
                        currentIndex: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerSideChainSourceDeviceKey()) + 1;
                        }
                        onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerSideChainSourceDeviceKey(), index - 1)
                    }
                    FilterKnob {
                        label: qsTr("LPF")
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.autoDuckerSideChainLpfKey()) * Constants.uiInternalScaling;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.autoDuckerSideChainLpfKey(), v / Constants.uiInternalScaling)
                    }
                }
            }

            // Gain Meter, bipolar around the zero line: down while ducking, up while boosting
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignTop
                Label {
                    text: qsTr("Gain")
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Rectangle {
                    id: meterContainer
                    width: 30
                    height: 300
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    Rectangle {
                        id: meterFill
                        property real half: (meterContainer.height - 4) / 2
                        property real magnitude: Math.min(1.0, Math.abs(root.currentGainDb) / 24.0) * half
                        width: parent.width - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: magnitude
                        y: root.currentGainDb >= 0 ? 2 + half - magnitude : 2 + half
                        color: themeService.accentColor
                    }

                    // Tick marks
                    Item {
                        anchors.fill: parent
                        anchors.margins: 2
                        Repeater {
                            model: 9 // +24, +18, +12, +6, 0, -6, -12, -18, -24
                            Rectangle {
                                width: parent.width
                                height: index === 4 ? 2 : 1
                                color: index === 4 ? "#888" : "#555"
                                opacity: index === 4 ? 0.9 : 0.5
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: (index / 8) * parent.height
                            }
                        }
                    }
                }
                Label {
                    id: gainLabel
                    text: (root.currentGainDb > 0 ? "+" : "") + root.currentGainDb.toFixed(1) + " dB"
                    color: themeService.accentColor
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                    // Reserve the widest reading the range can produce, or the column resizes as
                    // digits and the sign come and go and the knobs shift with it
                    Layout.preferredWidth: gainLabelMetrics.width
                    TextMetrics {
                        id: gainLabelMetrics
                        font: gainLabel.font
                        text: "-24.0 dB"
                    }
                }
            }
        } // RowLayout
    } // ColumnLayout

    property real currentGainDb: 0.0

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentGainDb = effectRackController.autoDuckerGainDb(root.effectIndex);
        }
    }
}
