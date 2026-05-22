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
    title: "<strong>" + qsTr("EQ 8-Band Parametric (Slot %1)").arg(effectIndex + 1) + "</strong>"
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: bandsRow.implicitWidth
            contentHeight: -1
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            RowLayout {
                id: bandsRow
                spacing: 40
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                Repeater {
                    model: 8
                    delegate: ColumnLayout {
                        spacing: 15
                        Layout.alignment: Qt.AlignTop
                        
                        Label {
                            text: "<strong>" + qsTr("Band %1").arg(index + 1) + "</strong>"
                            font.pointSize: 12
                            color: themeService.accentColor
                            Layout.alignment: Qt.AlignHCenter
                        }

                        BandSettings {
                            bandIndex: index
                            effectIndex: root.effectIndex
                        }
                    }
                }
            }
        }
    }

    component BandSettings: ColumnLayout {
        property int bandIndex: -1
        property int effectIndex: -1
        spacing: 15
        Layout.preferredWidth: 120

        ColumnLayout {
            spacing: 5
            Layout.alignment: Qt.AlignHCenter
            Label {
                text: qsTr("Type")
                font.bold: true
                font.pixelSize: 11
                color: "#aaa"
                Layout.alignment: Qt.AlignHCenter
            }
            ComboBox {
                id: typeCombo
                implicitWidth: 110
                model: [qsTr("Bypass"), qsTr("Bell"), qsTr("Low Shelf"), qsTr("High Shelf"), qsTr("Low Cut"), qsTr("High Cut"), qsTr("Notch")]
                currentIndex: {
                    effectRackController.revision;
                    return Math.round(effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricTypeKey(bandIndex)) * 6);
                }
                onActivated: index => effectRackController.setParameterValue(effectIndex, effectRackController.eq8BandParametricTypeKey(bandIndex), index / 6.0)
            }
        }

        Knob {
            label: qsTr("Freq")
            suffix: "Hz"
            mapping: "logFrequency"
            mapMin: 20
            mapMax: 20000
            from: 0
            to: 1000
            value: {
                effectRackController.revision;
                return effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricFreqKey(bandIndex)) * 1000;
            }
            onMoved: v => effectRackController.setParameterValue(effectIndex, effectRackController.eq8BandParametricFreqKey(bandIndex), v / 1000)
            Layout.fillWidth: true
            enabled: typeCombo.currentIndex !== 0
        }

        Knob {
            label: qsTr("Gain")
            suffix: "dB"
            from: -24
            to: 24
            value: {
                effectRackController.revision;
                return -24 + effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricGainKey(bandIndex)) * 48;
            }
            onMoved: v => effectRackController.setParameterValue(effectIndex, effectRackController.eq8BandParametricGainKey(bandIndex), (v + 24) / 48)
            Layout.fillWidth: true
            enabled: typeCombo.currentIndex === 1 || typeCombo.currentIndex === 2 || typeCombo.currentIndex === 3
        }

        Knob {
            label: qsTr("Q")
            suffix: ""
            mapping: "exponential"
            isInteger: false
            mapMin: 0.1
            mapMax: 10.0
            from: 0
            to: 1000
            value: {
                effectRackController.revision;
                return effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricQKey(bandIndex)) * 1000;
            }
            onMoved: v => effectRackController.setParameterValue(effectIndex, effectRackController.eq8BandParametricQKey(bandIndex), v / 1000)
            Layout.fillWidth: true
            enabled: typeCombo.currentIndex !== 0
        }
    }
}
