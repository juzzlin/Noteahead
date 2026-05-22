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
    title: "<strong>" + qsTr("EQ 32-Band Graphical (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    // 32 sliders want the width, so the large share of the window
    width: parent ? parent.width * Constants.largeDialogScale : 1000
    height: parent ? parent.height * Constants.largeDialogScale : 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    //! A slider position in dB, from the parameter's 0..1.
    function sliderDb(value: real): real {
        return value * 24 - 12;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

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
                implicitWidth: 160
                model: [qsTr("Stereo"), qsTr("Mid"), qsTr("Side")]
                currentIndex: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.eq32BandGraphicalStereoModeKey());
                }
                onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.eq32BandGraphicalStereoModeKey(), index)
            }
            AppButton {
                text: qsTr("Flat")
                toolTipText: qsTr("Every slider back to 0 dB")
                onClicked: effectRackController.eq32BandGraphicalFlatten(root.effectIndex)
            }
            Item {
                Layout.fillWidth: true
            }
            Knob {
                label: qsTr("Output")
                suffix: "dB"
                isInteger: false
                from: -12
                to: 12
                value: {
                    effectRackController.revision;
                    return root.sliderDb(effectRackController.parameterValue(root.effectIndex, effectRackController.eq32BandGraphicalOutputGainKey()));
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.eq32BandGraphicalOutputGainKey(), (v + 12) / 24)
                Layout.preferredWidth: 220
            }
        }

        EqCurveRenderer {
            // From the equalizer itself, so the curve is what is heard, the band interaction and the
            // output gain included
            response: {
                effectRackController.revision;
                return effectRackController.eq32BandGraphicalResponse(root.effectIndex, Math.max(64, Math.round(width / 2)));
            }
            dbRange: 15
            accentColor: themeService.accentColor
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            Layout.minimumHeight: 60
        }

        RowLayout {
            id: sliderRow
            spacing: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120

            Repeater {
                model: effectRackController.eq32BandGraphicalBandCount()
                delegate: BandSlider {
                    bandIndex: index
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 0
                }
            }
        }
    }

    component BandSlider: ColumnLayout {
        id: band
        property int bandIndex: -1
        readonly property string gainKey: effectRackController.eq32BandGraphicalBandGainKey(bandIndex)
        readonly property real gainDb: {
            effectRackController.revision;
            return root.sliderDb(effectRackController.parameterValue(root.effectIndex, gainKey));
        }
        function setGainDb(db: real): void {
            effectRackController.setParameterValue(root.effectIndex, gainKey, (Math.max(-12, Math.min(12, db)) + 12) / 24);
        }
        spacing: 2

        Label {
            // A tenth of a decibel is as fine as a slider this short can be read
            text: Math.abs(band.gainDb) < 0.05 ? "0" : (band.gainDb > 0 ? "+" : "") + band.gainDb.toFixed(1)
            font.pixelSize: 9
            color: Math.abs(band.gainDb) < 0.05 ? "#aaa" : themeService.accentColor
            horizontalAlignment: Text.AlignHCenter
            // Every child fills, or the column cannot grow past its narrowest width and the row
            // leaves the right end of the dialog empty
            Layout.fillWidth: true
        }
        Slider {
            id: slider
            orientation: Qt.Vertical
            from: -12
            to: 12
            stepSize: 0.1
            value: band.gainDb
            onMoved: band.setGainDb(value)
            Layout.fillHeight: true
            Layout.fillWidth: true
            Binding {
                target: slider
                property: "value"
                value: band.gainDb
                when: !slider.pressed
            }
            // Half a decibel a notch; Ctrl for a tenth, as on the knobs
            WheelHandler {
                onWheel: wheel => {
                    const step = (wheel.modifiers & Qt.ControlModifier) ? 0.1 : 0.5;
                    band.setGainDb(band.gainDb + Math.sign(wheel.angleDelta.y) * step);
                }
            }
            // Back to 0 dB, by a right-click as on every knob, or a double-click
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onPressed: band.setGainDb(0)
            }
            // A handler rather than the MouseArea: it watches the presses without taking them from
            // the slider, which a MouseArea would have to do to see a double-click at all
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onDoubleTapped: band.setGainDb(0)
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: effectRackController.eq32BandGraphicalBandLabel(band.bandIndex) + ": " + band.gainDb.toFixed(1) + " dB"
        }
        Label {
            // "1.25 kHz" does not fit under a slider this narrow; "1.25k" does
            text: effectRackController.eq32BandGraphicalBandLabel(band.bandIndex).replace(" kHz", "k").replace(" Hz", "")
            font.pixelSize: 9
            color: "#aaa"
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }
}
