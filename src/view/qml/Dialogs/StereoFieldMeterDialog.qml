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

    title: "<strong>" + qsTr("Stereo Field Meter (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    // How many sample pairs the goniometer is drawn from. Enough to show the shape of the field,
    // few enough that building the list thirty times a second stays cheap.
    readonly property int tracePoints: 512

    // Analysis is gated on the dialog being open, so a meter left in a rack costs nothing.
    onVisibleChanged: {
        if (effectIndex >= 0) {
            effectRackController.stereoFieldMeterSetActive(effectIndex, visible);
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.visible && root.effectIndex >= 0
        repeat: true
        onTriggered: {
            renderer.points = effectRackController.stereoFieldMeterPoints(root.effectIndex, root.tracePoints);
            const reading = effectRackController.stereoFieldMeterReading(root.effectIndex);
            if (reading.correlation !== undefined) {
                renderer.correlation = reading.correlation;
                renderer.bandCorrelations = reading.bandCorrelations;
                renderer.midDb = reading.midDb;
                renderer.sideDb = reading.sideDb;
                renderer.balance = reading.balance;
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        StereoFieldRenderer {
            id: renderer
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
            accentColor: themeService.accentColor
            zoom: {
                effectRackController.revision;
                return root.zoomFactor();
            }
            showGuides: {
                effectRackController.revision;
                return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoFieldMeterShowGuidesKey()) > 0.5;
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            ColumnLayout {
                spacing: 2
                Label {
                    text: qsTr("Speed")
                    color: "#aaa"
                    font.pixelSize: 11
                }
                ComboBox {
                    implicitWidth: 120
                    model: [qsTr("Fast"), qsTr("Normal"), qsTr("Slow")]
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.stereoFieldMeterSpeedKey()));
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoFieldMeterSpeedKey(), index)
                }
            }

            Knob {
                label: qsTr("Zoom")
                suffix: "x"
                Layout.fillWidth: false
                Layout.preferredWidth: 160
                mapping: "exponential"
                mapMin: 0.25
                mapMax: 4
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoFieldMeterZoomKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoFieldMeterZoomKey(), v / 1000)
            }

            Switch {
                text: qsTr("Guides")
                checked: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoFieldMeterShowGuidesKey()) > 0.5;
                }
                onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoFieldMeterShowGuidesKey(), checked ? 1 : 0)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Show the axes, amplitude rings and scales the readings are drawn against")
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    // The stored value is the knob's own position; the renderer wants the factor it stands for.
    function zoomFactor() {
        const position = effectRackController.parameterValue(root.effectIndex, effectRackController.stereoFieldMeterZoomKey());
        return knobController.map(position, "exponential", 0.25, 4);
    }
}
