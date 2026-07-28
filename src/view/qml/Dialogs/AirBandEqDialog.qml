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
    title: "<strong>" + qsTr("Air Band EQ (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

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

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        // The air band is a boost-only tap summed alongside the band passes, so raising it lifts the
        // overall level as well. That interaction is intentional; the Output trim below offsets it.
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            spacing: 15

            Label {
                text: "<strong>" + qsTr("Air Band") + "</strong>"
                font.pointSize: 12
                color: themeService.accentColor
                Layout.alignment: Qt.AlignHCenter
            }

            LabeledCombo {
                label: qsTr("Freq")
                model: [qsTr("Off"), "2.5 kHz", "5 kHz", "10 kHz", "20 kHz", "40 kHz"]
                paramKey: effectRackController.airBandEqAirFreqKey()
            }

            Knob {
                label: qsTr("Air Gain")
                suffix: ""
                isInteger: false
                from: 0
                to: 5
                Layout.fillWidth: true
                Layout.minimumWidth: 120
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.airBandEqAirGainKey()) * 5;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.airBandEqAirGainKey(), v / 5)
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: "#333"
        }

        // The five band passes are summed in parallel with the dry signal, so they interact: pulling
        // them all down by the same amount lowers the whole curve without changing its shape.
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            spacing: 15

            Label {
                text: "<strong>" + qsTr("Band Passes") + "</strong>"
                font.pointSize: 12
                color: themeService.accentColor
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                spacing: 15
                Layout.fillWidth: true

                BandKnob {
                    label: qsTr("Sub")
                    bandIndex: 0
                }
                BandKnob {
                    label: "40 Hz"
                    bandIndex: 1
                }
                BandKnob {
                    label: "160 Hz"
                    bandIndex: 2
                }
                BandKnob {
                    label: "650 Hz"
                    bandIndex: 3
                }
                BandKnob {
                    label: "2.5 kHz"
                    bandIndex: 4
                }
            }

            Knob {
                label: qsTr("Output")
                suffix: "dB"
                isInteger: false
                from: -12
                to: 12
                Layout.fillWidth: true
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.airBandEqOutputGainKey()) * 24 - 12;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.airBandEqOutputGainKey(), (v + 12) / 24)
            }
        }
    }

    // A band-pass knob in panel units: detented -5..+5 about a flat centre. The dB it maps to is
    // deliberately asymmetric, +15 up against -4.5 down, which the parallel summing makes unavoidable.
    component BandKnob: Knob {
        property int bandIndex: 0
        suffix: ""
        isInteger: false
        from: -5
        to: 5
        Layout.fillWidth: true
        value: {
            effectRackController.revision;
            return effectRackController.parameterValue(root.effectIndex, effectRackController.airBandEqBandGainKey(bandIndex)) * 10 - 5;
        }
        onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.airBandEqBandGainKey(bandIndex), (v + 5) / 10)
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
