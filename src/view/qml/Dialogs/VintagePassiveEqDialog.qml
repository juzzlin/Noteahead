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
    title: "<strong>" + qsTr("Vintage Passive EQ (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        RowLayout {
            width: dialogScrollView.availableWidth
            spacing: 30

            // Low-frequency section: shared frequency selector with independent boost and attenuation.
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                spacing: 15

                Label {
                    text: "<strong>" + qsTr("Low Frequency") + "</strong>"
                    font.pointSize: 12
                    color: themeService.accentColor
                    Layout.alignment: Qt.AlignHCenter
                }

                LabeledCombo {
                    label: qsTr("Freq")
                    model: ["20 Hz", "30 Hz", "60 Hz", "100 Hz"]
                    paramKey: effectRackController.vintagePassiveEqLowFreqKey()
                }

                RowLayout {
                    spacing: 15
                    Layout.alignment: Qt.AlignHCenter

                    Knob {
                        label: qsTr("Boost")
                        suffix: "dB"
                        from: 0
                        to: 14
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.vintagePassiveEqLowBoostKey()) * 14;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.vintagePassiveEqLowBoostKey(), v / 14)
                    }
                    Knob {
                        label: qsTr("Atten")
                        suffix: "dB"
                        from: 0
                        to: 14
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.vintagePassiveEqLowAttenKey()) * 14;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.vintagePassiveEqLowAttenKey(), v / 14)
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: "#333"
            }

            // High-frequency section: a bell boost with a bandwidth control plus a separate shelf attenuator.
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                spacing: 15

                Label {
                    text: "<strong>" + qsTr("High Frequency") + "</strong>"
                    font.pointSize: 12
                    color: themeService.accentColor
                    Layout.alignment: Qt.AlignHCenter
                }

                LabeledCombo {
                    label: qsTr("Boost Freq")
                    model: ["3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz"]
                    paramKey: effectRackController.vintagePassiveEqHighBoostFreqKey()
                }

                RowLayout {
                    spacing: 15
                    Layout.alignment: Qt.AlignHCenter

                    Knob {
                        label: qsTr("Boost")
                        suffix: "dB"
                        from: 0
                        to: 18
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.vintagePassiveEqHighBoostKey()) * 18;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.vintagePassiveEqHighBoostKey(), v / 18)
                    }
                    Knob {
                        label: qsTr("Bandwidth")
                        suffix: "Q"
                        mapping: "exponential"
                        isInteger: false
                        mapMin: 0.3
                        mapMax: 3.0
                        from: 0
                        to: 1000
                        value: {
                            effectRackController.revision;
                            return effectRackController.parameterValue(root.effectIndex, effectRackController.vintagePassiveEqBandwidthKey()) * 1000;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.vintagePassiveEqBandwidthKey(), v / 1000)
                    }
                }

                LabeledCombo {
                    label: qsTr("Atten Freq")
                    model: ["5 kHz", "10 kHz", "20 kHz"]
                    paramKey: effectRackController.vintagePassiveEqHighAttenFreqKey()
                }

                Knob {
                    label: qsTr("Atten")
                    suffix: "dB"
                    from: 0
                    to: 18
                    Layout.alignment: Qt.AlignHCenter
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.vintagePassiveEqHighAttenKey()) * 18;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.vintagePassiveEqHighAttenKey(), v / 18)
                }
            }
        }
    }

    // A labeled discrete selector bound to an effect parameter that stores the selected index directly.
    component LabeledCombo: ColumnLayout {
        property alias label: caption.text
        property alias model: combo.model
        property string paramKey: ""
        spacing: 5
        Layout.alignment: Qt.AlignHCenter

        Label {
            id: caption
            font.bold: true
            font.pixelSize: 11
            color: "#aaa"
            Layout.alignment: Qt.AlignHCenter
        }
        ComboBox {
            id: combo
            implicitWidth: 120
            currentIndex: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, paramKey);
            }
            onActivated: index => effectRackController.setParameterValue(root.effectIndex, paramKey, index)
        }
    }
}
