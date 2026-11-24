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
    title: "<strong>" + qsTr("Resonance Tamer (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 650

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    readonly property int linkedMode: 0
    readonly property int stereoMode: {
        effectRackController.revision;
        return Math.round(root.parameter(NahdXml.stereoMode));
    }
    readonly property var bellCounts: [6, 12, 24]
    property real deepestCutDb: 0.0
    property var primaryReduction: []
    property var secondaryReduction: []

    function parameter(key: string): real {
        return effectRackController.parameterValue(root.effectIndex, key);
    }
    function setParameter(key: string, value: real): void {
        effectRackController.setParameterValue(root.effectIndex, key, value);
    }

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: {
            const points = Math.max(64, Math.round(curve.width / 2));
            root.deepestCutDb = dialogModel.deepestCutDb();
            root.primaryReduction = dialogModel.reduction(0, points);
            root.secondaryReduction = root.stereoMode === root.linkedMode ? [] : dialogModel.reduction(1, points);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            EffectPresetRow {
                effectIndex: root.effectIndex
            }
            Label {
                text: qsTr("Spectrum")
                font.bold: true
            }
            SpectrumModeComboBox {
                effectIndex: root.effectIndex
                renderer: curve
                active: root.visible
                ToolTip.text: qsTr("The live spectrum behind the curve. Pre − Post shows what is being taken out.")
            }
        }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            Label {
                text: qsTr("Stereo Mode")
                font.bold: true
            }
            ComboBox {
                implicitWidth: 120
                model: [qsTr("Linked"), qsTr("Stereo"), "Mid/Side"]
                currentIndex: root.stereoMode
                onActivated: index => root.setParameter(NahdXml.stereoMode, index)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Linked hears both sides and cuts both alike. Stereo and Mid/Side find and cut the peaks of each side on its own.")
            }
            Label {
                text: qsTr("Quality")
                font.bold: true
            }
            ComboBox {
                implicitWidth: 120
                model: [qsTr("%1 bells").arg(6), qsTr("%1 bells").arg(12), qsTr("%1 bells").arg(24)]
                currentIndex: {
                    effectRackController.revision;
                    return Math.max(0, root.bellCounts.indexOf(Math.round(root.parameter(NahdXml.bandCount))));
                }
                onActivated: index => root.setParameter(NahdXml.bandCount, root.bellCounts[index])
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("How many peaks it may cut at once. More bells catch more of a busy spectrum and cost more.")
            }
            Knob {
                label: qsTr("Mix")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return root.parameter(NahdXml.mix) * 100;
                }
                onMoved: v => root.setParameter(NahdXml.mix, v / 100)
                Layout.fillWidth: true
                Layout.maximumWidth: 260
            }
            Switch {
                text: qsTr("Delta")
                checked: {
                    effectRackController.revision;
                    return root.parameter(NahdXml.solo) > 0.5;
                }
                onToggled: root.setParameter(NahdXml.solo, checked ? 1 : 0)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Play only what is being taken out")
            }
        }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 90

            Label {
                // The live cut, and in Stereo or Mid/Side the right or side one dashed
                text: qsTr("Cutting now") + " " + root.deepestCutDb.toFixed(1) + " dB" + (root.stereoMode === root.linkedMode ? "" : "   " + (root.stereoMode === 1 ? qsTr("(solid: left, dashed: right)") : qsTr("(solid: mid, dashed: side)")))
                color: root.deepestCutDb < -0.05 ? themeService.accentColor : Theme.textSecondary
                font.family: "Monospace"
            }
            EqCurveView {
                id: curve
                response: root.primaryReduction
                secondaryResponse: root.secondaryReduction
                dbRange: 18
                accentColor: themeService.accentColor
                spectrumPostColor: themeService.spectrumPostColor
                spectrumPreColor: themeService.spectrumPreColor
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        ScrollView {
            id: parametersScrollView
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            // Gives up height to the curve in a short dialog, and scrolls instead
            Layout.fillHeight: true
            Layout.preferredHeight: parametersGrid.implicitHeight
            Layout.maximumHeight: parametersGrid.implicitHeight
            Layout.minimumHeight: 100

            GridLayout {
                id: parametersGrid
                width: parametersScrollView.availableWidth
                columns: 4
                columnSpacing: 20
                rowSpacing: 10

                Knob {
                    label: qsTr("Depth")
                    suffix: "dB"
                    isInteger: false
                    from: 0
                    to: 24
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.depth) * 24;
                    }
                    onMoved: v => root.setParameter(NahdXml.depth, v / 24)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The most taken out of any one peak")
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Selectivity")
                    suffix: "dB"
                    isInteger: false
                    from: 1
                    to: 18
                    value: {
                        effectRackController.revision;
                        return 1 + root.parameter(NahdXml.selectivity) * 17;
                    }
                    onMoved: v => root.setParameter(NahdXml.selectivity, (v - 1) / 17)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far a peak must stand above the spectrum's outline before anything is taken out. Lower catches more.")
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Sharpness")
                    suffix: ""
                    mapping: "exponential"
                    isInteger: false
                    mapMin: 1
                    mapMax: 20
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.sharpness) * 1000;
                    }
                    onMoved: v => root.setParameter(NahdXml.sharpness, v / 1000)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The Q of the cuts: higher takes out narrower slices")
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Focus")
                    suffix: "%"
                    mapping: "bipolar"
                    mapMin: -100
                    mapMax: 100
                    from: -100
                    to: 100
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.focus) * 200 - 100;
                    }
                    onMoved: v => root.setParameter(NahdXml.focus, (v + 100) / 200)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Positive makes it more eager towards the top and less towards the bottom, negative the other way round")
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Attack")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 1
                    mapMax: 200
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.attack) * 1000;
                    }
                    onMoved: v => root.setParameter(NahdXml.attack, v / 1000)
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Release")
                    suffix: "ms"
                    mapping: "exponential"
                    mapMin: 10
                    mapMax: 1000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.release) * 1000;
                    }
                    onMoved: v => root.setParameter(NahdXml.release, v / 1000)
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("Low")
                    suffix: "Hz"
                    mapping: "exponentialHz"
                    mapMin: 20
                    mapMax: 20000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.lowFreq) * 1000;
                    }
                    onMoved: v => root.setParameter(NahdXml.lowFreq, v / 1000)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Peaks below this are left alone")
                    Layout.fillWidth: true
                }
                Knob {
                    label: qsTr("High")
                    suffix: "Hz"
                    mapping: "exponentialHz"
                    mapMin: 20
                    mapMax: 20000
                    from: 0
                    to: 1000
                    value: {
                        effectRackController.revision;
                        return root.parameter(NahdXml.highFreq) * 1000;
                    }
                    onMoved: v => root.setParameter(NahdXml.highFreq, v / 1000)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Peaks above this are left alone")
                    Layout.fillWidth: true
                }
            }
        }
    }

    ResonanceTamerDialogModel {
        id: dialogModel
        rack: effectRackController
        effectIndex: root.effectIndex
    }
}
