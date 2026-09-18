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
    title: "<strong>" + qsTr("EQ 8-Band Parametric (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    // Main.qml keeps one instance of this dialog and hands it whichever slot is being edited, so
    // the band grid would otherwise open wherever the previous visit left it scrolled. Put back
    // before the dialog is shown rather than after, so there is nothing to see jumping. EffectDialog
    // takes its own snapshot through a Connections precisely so that a handler here does not
    // displace it.
    onAboutToShow: {
        scrollView.contentItem.contentX = 0;
        scrollView.contentItem.contentY = 0;
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        EffectPresetRow {
            effectIndex: root.effectIndex
        }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            Label {
                text: qsTr("Stereo Mode")
                font.bold: true
            }
            ComboBox {
                id: stereoModeCombo
                implicitWidth: 160
                model: [qsTr("Mid + Side"), qsTr("Mid"), qsTr("Side")]
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.eq8BandParametricStereoModeKey());
                }
                onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.eq8BandParametricStereoModeKey(), index)
            }
            EqCurveRenderer {
                // The response comes from the equalizer itself, which reads it off the filters it
                // processes with, so the curve cannot drift from what is being heard. revision is
                // bumped by every parameter change, which is what makes it follow a knob.
                response: {
                    effectRackController.revision;
                    // One point per couple of pixels, so the curve stays smooth at whatever width
                    // the dialog gives it. Depending on the width here is also what re-reads it when
                    // the dialog is resized.
                    return effectRackController.eq8BandParametricResponse(root.effectIndex, Math.max(64, Math.round(width / 2)));
                }
                // Flat, and drawn only when the stereo mode above leaves one path unshaped. Without
                // it a Mid or Side cut looks exactly like one taken across the whole image, which is
                // the one thing about those modes worth seeing.
                secondaryResponse: {
                    effectRackController.revision;
                    return effectRackController.eq8BandParametricPassThroughResponse(root.effectIndex, Math.max(64, Math.round(width / 2)));
                }
                dbRange: 18
                accentColor: themeService.accentColor
                Layout.fillWidth: true
                Layout.preferredHeight: 120
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            GridLayout {
                id: bandsGrid
                columns: 4
                rowSpacing: 30
                columnSpacing: 30
                // availableWidth rather than the Flickable's own width, which is what the grid's
                // parent is: available is what is left once the vertical scroll bar has taken its
                // share, so the fourth column does not run under the bar. The flick range is left
                // to the layout's implicit size for the same reason -- pinned at the grid's
                // unsqueezed implicit width, it was wider than the viewport, which gives the view
                // somewhere to sit other than the start.
                width: scrollView.availableWidth

                Repeater {
                    model: 8
                    delegate: ColumnLayout {
                        spacing: 15
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        
                        Label {
                            text: "<strong>" + qsTr("Band %1").arg(index + 1) + "</strong>"
                            font.pointSize: 12
                            color: themeService.accentColor
                            Layout.alignment: Qt.AlignHCenter
                        }

                        BandSettings {
                            bandIndex: index
                            effectIndex: root.effectIndex
                            Layout.fillWidth: true
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

        RowLayout {
            spacing: 10
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Label {
                    text: qsTr("Type")
                    font.bold: true
                    font.pixelSize: 11
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
                ComboBox {
                    id: typeCombo
                    model: [qsTr("Bypass"), qsTr("Bell"), qsTr("Low Shelf"), qsTr("High Shelf"), qsTr("Low Cut"), qsTr("High Cut"), qsTr("Notch")]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricTypeKey(bandIndex));
                    }
                    // Q means a different thing to each type, so picking one opens it at the width
                    // that type is reached for at. Which type it already was decides whether that
                    // happens at all, so the controller is asked rather than told.
                    onActivated: index => effectRackController.eq8BandParametricSetBandType(effectIndex, bandIndex, index)
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Label {
                    text: qsTr("Slope")
                    font.bold: true
                    font.pixelSize: 11
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
                ComboBox {
                    model: ["12 dB/oct", "24 dB/oct", "48 dB/oct"]
                    currentIndex: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(effectIndex, effectRackController.eq8BandParametricSlopeKey(bandIndex));
                    }
                    onActivated: index => effectRackController.setParameterValue(effectIndex, effectRackController.eq8BandParametricSlopeKey(bandIndex), index)
                    // Only a cut has a slope to set. A bell or a shelf is shaped by its Q.
                    enabled: typeCombo.currentIndex === 4 || typeCombo.currentIndex === 5
                    Layout.fillWidth: true
                }
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
