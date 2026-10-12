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
    title: "<strong>" + qsTr("Dynamic EQ (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 650

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    readonly property int flatType: 3
    readonly property int bandType: {
        effectRackController.revision;
        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.dynamicEqTypeKey()));
    }
    readonly property bool externalDetector: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, effectRackController.dynamicEqDetectorSourceKey()) >= 0.5;
    }
    readonly property bool bandDetector: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, effectRackController.dynamicEqDetectorFilterKey()) < 0.5;
    }
    property real currentGainDb: 0.0
    property real detectorDb: -120.0
    readonly property real thresholdDb: {
        effectRackController.revision;
        return -60 + root.parameter(effectRackController.dynamicEqThresholdKey()) * 60;
    }
    property var gainHistory: []
    property var liveResponse: []

    function parameter(key: string): real {
        return effectRackController.parameterValue(root.effectIndex, key);
    }
    function setParameter(key: string, value: real): void {
        effectRackController.setParameterValue(root.effectIndex, key, value);
    }

    onVisibleChanged: effectRackController.dynamicEqSetActive(root.effectIndex, visible)

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentGainDb = effectRackController.dynamicEqGainDb(root.effectIndex);
            root.detectorDb = effectRackController.dynamicEqDetectorLevelDb(root.effectIndex);
            root.gainHistory = effectRackController.dynamicEqGainHistory(root.effectIndex);
            root.liveResponse = effectRackController.dynamicEqResponse(root.effectIndex, Math.max(64, Math.round(curve.width / 2)));
            historyCanvas.requestPaint();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        EffectPresetRow {
            effectIndex: root.effectIndex
        }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            Label {
                text: qsTr("Type")
                font.bold: true
            }
            ComboBox {
                implicitWidth: 140
                model: [qsTr("Bell"), qsTr("Low Shelf"), qsTr("High Shelf"), qsTr("Flat")]
                currentIndex: root.bandType
                onActivated: index => root.setParameter(effectRackController.dynamicEqTypeKey(), index)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Flat moves the whole signal rather than a band of it, which makes a ducker driven by what the detector hears")
            }
            Label {
                text: qsTr("Stereo Mode")
                font.bold: true
            }
            ComboBox {
                implicitWidth: 120
                model: [qsTr("Stereo"), qsTr("Mid"), qsTr("Side")]
                currentIndex: {
                    effectRackController.revision;
                    return root.parameter(effectRackController.dynamicEqStereoModeKey());
                }
                onActivated: index => root.setParameter(effectRackController.dynamicEqStereoModeKey(), index)
            }
            CheckBox {
                text: qsTr("Listen")
                checked: {
                    effectRackController.revision;
                    return root.parameter(effectRackController.dynamicEqListenKey()) >= 0.5;
                }
                onToggled: root.setParameter(effectRackController.dynamicEqListenKey(), checked ? 1 : 0)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Play what the detector hears instead of the output, to find the frequency to react to")
            }
        }

        RowLayout {
            // The band on the left, and on the right how it has been moving and what is moving it
            spacing: 10
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 90

            EqCurveRenderer {
                id: curve
                // Where the band is now, and dashed, where the whole of its Gain would take it
                response: root.liveResponse
                secondaryResponse: {
                    effectRackController.revision;
                    return effectRackController.dynamicEqRangeResponse(root.effectIndex, Math.max(64, Math.round(width / 2)));
                }
                dbRange: 18
                accentColor: themeService.accentColor
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                Layout.fillHeight: true
            }
            ColumnLayout {
                spacing: 4
                Layout.preferredWidth: 280
                Layout.maximumWidth: 280
                Layout.fillHeight: true

                ColumnLayout {
                    spacing: 2
                    Label {
                        text: qsTr("Gain now") + " " + (root.currentGainDb > 0.05 ? "+" : "") + root.currentGainDb.toFixed(1) + " dB"
                        color: themeService.accentColor
                        font.family: "Monospace"
                    }
                    Label {
                        // What the detector hears against the threshold: a side chain that never
                        // crosses the line is why a band does not move
                        text: qsTr("Detector") + " " + (root.detectorDb <= -100 ? "-inf" : root.detectorDb.toFixed(1)) + " dB"
                        color: root.detectorDb > root.thresholdDb ? themeService.accentColor : "#aaa"
                        font.family: "Monospace"
                    }
                }
                RowLayout {
                    spacing: 6
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Canvas {
                        id: historyCanvas
                        // The band's gain over the last couple of seconds, newest on the right: the attack
                        // and the release as they happen, between the flat line and the Gain
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.fillStyle = "#111";
                            ctx.fillRect(0, 0, width, height);
                            const range = effectRackController.dynamicEqRangeDb(root.effectIndex);
                            const span = Math.max(1, Math.abs(range));
                            // Flat at the top for a cut, at the bottom for a boost
                            const zeroY = range < 0 ? 2 : height - 2;
                            const yOf = db => zeroY - db / span * (height - 4);
                            ctx.strokeStyle = "#444";
                            ctx.lineWidth = 1;
                            ctx.beginPath();
                            ctx.moveTo(0, zeroY);
                            ctx.lineTo(width, zeroY);
                            ctx.stroke();
                            const history = root.gainHistory;
                            if (history.length < 2) {
                                return;
                            }
                            ctx.strokeStyle = themeService.accentColor;
                            ctx.lineWidth = 2;
                            ctx.beginPath();
                            for (let i = 0; i < history.length; i++) {
                                const x = i / (history.length - 1) * width;
                                const y = Math.max(0, Math.min(height, yOf(history[i])));
                                if (i === 0) {
                                    ctx.moveTo(x, y);
                                } else {
                                    ctx.lineTo(x, y);
                                }
                            }
                            ctx.stroke();
                        }
                    }
                    Rectangle {
                        id: detectorMeter
                        // The detector level, -60 to 0 dB, with the threshold marked in white
                        readonly property real floorDb: -60
                        function yOf(db: real): real {
                            return (1 - Math.max(0, Math.min(1, (db - floorDb) / -floorDb))) * height;
                        }
                        color: "#111"
                        border.color: "#333"
                        Layout.preferredWidth: 14
                        Layout.fillHeight: true
                        Rectangle {
                            x: 1
                            width: parent.width - 2
                            y: detectorMeter.yOf(root.detectorDb)
                            height: Math.max(0, parent.height - 1 - y)
                            color: root.detectorDb > root.thresholdDb ? themeService.accentColor : "#666"
                        }
                        Rectangle {
                            y: detectorMeter.yOf(root.thresholdDb) - 1
                            width: parent.width
                            height: 2
                            color: "white"
                        }
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: detectorMeterHover.hovered
                        ToolTip.text: qsTr("What the detector hears, against the threshold in white. The band moves only while this is over the line.")
                        HoverHandler {
                            id: detectorMeterHover
                        }
                    }
                }
            }
        }

        ScrollView {
            id: parametersScrollView
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            // Gives up height to the curve and the trace in a short dialog, and scrolls instead
            Layout.fillHeight: true
            Layout.preferredHeight: parametersColumn.implicitHeight
            Layout.maximumHeight: parametersColumn.implicitHeight
            Layout.minimumHeight: 100

            ColumnLayout {
                id: parametersColumn
                width: parametersScrollView.availableWidth
                spacing: 12

                GridLayout {
                    columns: 5
                    columnSpacing: 20
                    rowSpacing: 10
                    Layout.fillWidth: true

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
                            return root.parameter(effectRackController.dynamicEqFrequencyKey()) * 1000;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqFrequencyKey(), v / 1000)
                        // A flat band has no frequency of its own, but a band detector still listens at one
                        enabled: root.bandType !== root.flatType || root.bandDetector
                        Layout.fillWidth: true
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
                            return root.parameter(effectRackController.dynamicEqQKey()) * 1000;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqQKey(), v / 1000)
                        enabled: root.bandType !== root.flatType || root.bandDetector
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Gain")
                        suffix: "dB"
                        isInteger: false
                        from: -24
                        to: 24
                        value: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqRangeKey()) * 48 - 24;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqRangeKey(), (v + 24) / 48)
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("How far the band can move: negative cuts what gets too loud, positive brings it up")
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
                            return root.parameter(effectRackController.dynamicEqGainKey()) * 24 - 12;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqGainKey(), (v + 12) / 24)
                        Layout.fillWidth: true
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Threshold")
                        suffix: "dB"
                        from: -60
                        to: 0
                        value: {
                            effectRackController.revision;
                            return -60 + root.parameter(effectRackController.dynamicEqThresholdKey()) * 60;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqThresholdKey(), (v + 60) / 60)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Ratio")
                        suffix: ":1"
                        from: 1
                        to: 20
                        value: {
                            effectRackController.revision;
                            return 1 + root.parameter(effectRackController.dynamicEqRatioKey()) * 19;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqRatioKey(), (v - 1) / 19)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Knee")
                        suffix: "dB"
                        from: 0
                        to: 24
                        value: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqKneeKey()) * 24;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqKneeKey(), v / 24)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Attack")
                        suffix: "ms"
                        mapping: "exponential"
                        mapMin: 0.1
                        mapMax: 500
                        from: 0
                        to: 1000
                        value: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqAttackKey()) * 1000;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqAttackKey(), v / 1000)
                        Layout.fillWidth: true
                    }
                    Knob {
                        label: qsTr("Release")
                        suffix: "ms"
                        mapping: "exponential"
                        mapMin: 1.0
                        mapMax: 2000
                        from: 0
                        to: 1000
                        value: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqReleaseKey()) * 1000;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqReleaseKey(), v / 1000)
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Detector:")
                        font.bold: true
                    }
                    ComboBox {
                        implicitWidth: 130
                        model: [qsTr("Self"), qsTr("Side Chain")]
                        currentIndex: root.externalDetector ? 1 : 0
                        onActivated: index => root.setParameter(effectRackController.dynamicEqDetectorSourceKey(), index)
                    }
                    ComboBox {
                        implicitWidth: 130
                        model: [qsTr("Band only"), qsTr("Full range")]
                        currentIndex: root.bandDetector ? 0 : 1
                        onActivated: index => root.setParameter(effectRackController.dynamicEqDetectorFilterKey(), index)
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Band only hears what is around the band's frequency, through a band-pass of the same Freq and Q; Full range hears the whole signal")
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        enabled: root.externalDetector
                        model: {
                            // A JS block is not a translation binding, so a language change does not re-evaluate it
                            // on its own. Reading the active language makes the dependency explicit.
                            languageService.activeLanguage;
                            deviceRackController.revision;
                            var items = [qsTr("None")];
                            for (var i = 0; i < deviceRackController.deviceCount; i++) {
                                const typeName = deviceRackController.deviceTypeName(i);
                                const trackNames = deviceRackController.trackNames(i);
                                if (typeName === "") {
                                    items.push(qsTr("Device %1").arg(i + 1));
                                } else if (trackNames === "") {
                                    items.push(qsTr("Device %1: %2").arg(i + 1).arg(typeName));
                                } else {
                                    items.push(qsTr("Device %1: %2 — %3").arg(i + 1).arg(typeName).arg(trackNames));
                                }
                            }
                            return items;
                        }
                        currentIndex: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqSideChainSourceDeviceKey()) + 1;
                        }
                        onActivated: index => root.setParameter(effectRackController.dynamicEqSideChainSourceDeviceKey(), index - 1)
                    }
                    FilterKnob {
                        label: qsTr("LPF")
                        value: {
                            effectRackController.revision;
                            return root.parameter(effectRackController.dynamicEqSideChainLpfKey()) * Constants.uiInternalScaling;
                        }
                        onMoved: v => root.setParameter(effectRackController.dynamicEqSideChainLpfKey(), v / Constants.uiInternalScaling)
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Low-pass the detector, so that it reacts to the low end and ignores the top")
                        Layout.preferredWidth: 200
                        Layout.maximumWidth: 200
                    }
                }
            }
        }
    }
}
