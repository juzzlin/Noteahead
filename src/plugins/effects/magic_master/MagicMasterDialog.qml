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
    title: "<strong>" + qsTr("Magic Master (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.defaultDialogScale : 880
    height: parent ? parent.height * Constants.defaultDialogScale : 640

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    property real currentShortTermLufs: -70.0
    property real currentIntegratedLufs: -70.0
    property real currentTruePeakHoldDb: -70.0
    property real currentDriveDb: 0.0
    property bool currentCalibrated: false
    property string currentSummary: ""

    property real currentSubHpfHz: 28.0
    property real currentMonoBassHz: 125.0
    property real currentAutoEqMoveDb: 0.0
    property real currentResCutDb: 0.0
    property real currentMbLowGrDb: 0.0
    property real currentMbMidGrDb: 0.0
    property real currentMbHighGrDb: 0.0
    property real currentStereoCorr: 1.0
    property real currentStereoWidthPct: 100.0
    property real currentClipGrDb: 0.0
    property real currentLimGrDb: 0.0
    property string calibrationStatusMessage: ""

    readonly property real peakLimitDb: {
        effectRackController.revision;
        return -6.0 + effectRackController.parameterValue(root.effectIndex, NahdXml.ceiling) * 6.0;
    }

    readonly property bool subEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.subCleanEnabled) > 0.5;
    }
    readonly property bool eqEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.eqEnabled) > 0.5;
    }
    readonly property bool mbEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.multibandEnabled) > 0.5;
    }
    readonly property bool stereoEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.stereoEnabled) > 0.5;
    }
    readonly property bool clipperEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.clipperEnabled) > 0.5;
    }
    readonly property bool limiterEnabled: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, NahdXml.limiterEnabled) > 0.5;
    }

    function refreshTelemetry(): void {
        root.currentShortTermLufs = mmModel.shortTermLufs();
        root.currentIntegratedLufs = mmModel.integratedLufs();
        root.currentTruePeakHoldDb = mmModel.truePeakHoldDb();
        root.currentDriveDb = mmModel.appliedDriveDb();
        root.currentCalibrated = mmModel.isCalibrated();
        root.currentSummary = mmModel.songContextSummary();

        root.currentSubHpfHz = mmModel.subHpfHz();
        root.currentMonoBassHz = mmModel.monoBassHz();
        root.currentAutoEqMoveDb = mmModel.autoEqMaxMoveDb();
        root.currentResCutDb = mmModel.resonanceCutDb();
        root.currentMbLowGrDb = mmModel.multibandReductionDb(0);
        root.currentMbMidGrDb = mmModel.multibandReductionDb(1);
        root.currentMbHighGrDb = mmModel.multibandReductionDb(2);
        root.currentStereoCorr = mmModel.stereoCorrelation();
        root.currentStereoWidthPct = mmModel.stereoWidthPercent();
        root.currentClipGrDb = mmModel.clipperReductionDb();
        root.currentLimGrDb = mmModel.limiterReductionDb();
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 4

            // 1. Preset row
            EffectPresetRow {
                effectIndex: root.effectIndex
                showWhenEmpty: true
                Layout.fillWidth: true
            }

            // 2. Hero Controls & Master Readout
            SectionBox {
                title: qsTr("Master Target & Output")
                Layout.fillWidth: true
                bottomPadding: 4

                RowLayout {
                    anchors.fill: parent
                    spacing: 12

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Layout.minimumWidth: 125
                        Layout.maximumWidth: 340
                        Layout.alignment: Qt.AlignVCenter

                        Knob {
                            label: qsTr("LUFS Target")
                            mapping: "value"
                            suffix: "LUFS"
                            from: -18
                            to: -5
                            stepSize: 0.1
                            isInteger: false
                            Layout.fillWidth: true
                            value: {
                                effectRackController.revision;
                                return -18 + effectRackController.parameterValue(root.effectIndex, NahdXml.targetLufs) * 13;
                            }
                            onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.targetLufs, (v + 18) / 13)
                        }

                        CheckBox {
                            text: qsTr("Auto LUFS")
                            topPadding: 0
                            bottomPadding: 0
                            checked: {
                                effectRackController.revision;
                                return effectRackController.parameterValue(root.effectIndex, NahdXml.autoLoudnessEnabled) > 0.5;
                            }
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.autoLoudnessEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    Knob {
                        label: qsTr("Peak Limit")
                        mapping: "value"
                        suffix: "dBTP"
                        from: -6
                        to: 0
                        stepSize: 0.1
                        isInteger: false
                        Layout.fillWidth: true
                        Layout.minimumWidth: 125
                        Layout.maximumWidth: 340
                        Layout.alignment: Qt.AlignVCenter
                        value: {
                            effectRackController.revision;
                            return -6 + effectRackController.parameterValue(root.effectIndex, NahdXml.ceiling) * 6;
                        }
                        onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.ceiling, (v + 6) / 6)
                    }

                    Rectangle {
                        width: 1
                        Layout.fillHeight: true
                        color: Theme.separator
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 3
                        Layout.alignment: Qt.AlignVCenter

                        Label {
                            text: qsTr("Short-Term:")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        Label {
                            text: root.currentShortTermLufs <= -69.5 ? "--- LUFS" : root.currentShortTermLufs.toFixed(1) + " LUFS"
                            color: Theme.textBright
                            font.family: "Monospace"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            text: qsTr("Integrated:")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        Label {
                            text: root.currentIntegratedLufs <= -69.5 ? "--- LUFS" : root.currentIntegratedLufs.toFixed(1) + " LUFS"
                            color: Theme.textBright
                            font.family: "Monospace"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            text: qsTr("True Peak:")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        Label {
                            text: root.currentTruePeakHoldDb <= -69.5 ? "--- dBTP" : (root.currentTruePeakHoldDb >= 0 ? "+" : "") + root.currentTruePeakHoldDb.toFixed(1) + " dBTP"
                            color: root.currentTruePeakHoldDb > root.peakLimitDb + 0.1 ? Theme.danger : Theme.textBright
                            font.family: "Monospace"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            text: qsTr("Auto Drive:")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        RowLayout {
                            spacing: 6
                            Label {
                                text: (root.currentDriveDb >= 0 ? "+" : "") + root.currentDriveDb.toFixed(1) + " dB"
                                font.family: "Monospace"
                                font.pixelSize: 11
                                color: themeService.accentColor
                            }
                            Rectangle {
                                radius: 2
                                color: root.currentCalibrated ? themeService.accentColor : Theme.surfaceSunken
                                border.color: Theme.borderSubtle
                                implicitWidth: modeBadgeLabel.implicitWidth + 10
                                implicitHeight: modeBadgeLabel.implicitHeight + 4
                                Label {
                                    id: modeBadgeLabel
                                    anchors.centerIn: parent
                                    text: root.currentCalibrated ? qsTr("Calibrated") : qsTr("Live")
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: root.currentCalibrated ? Theme.surfaceSunken : Theme.textSecondary
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        Label {
                            text: qsTr("GR")
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.textSecondary
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            width: 16
                            height: 50
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            Layout.alignment: Qt.AlignHCenter
                            Rectangle {
                                anchors.top: parent.top
                                anchors.topMargin: 2
                                width: parent.width - 4
                                anchors.horizontalCenter: parent.horizontalCenter
                                readonly property real totalGr: Math.abs(root.currentClipGrDb) + Math.abs(root.currentLimGrDb)
                                height: Math.min(parent.height - 4, (totalGr / 24.0) * (parent.height - 4))
                                color: themeService.accentColor
                            }
                        }
                        Label {
                            readonly property real totalGr: -(Math.abs(root.currentClipGrDb) + Math.abs(root.currentLimGrDb))
                            text: totalGr.toFixed(1)
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: themeService.accentColor
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // 3. Song Context Bar & Actions
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 28
                    color: Theme.surfaceSunken
                    border.color: Theme.borderSubtle
                    radius: 2

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        verticalAlignment: Text.AlignVCenter
                        text: root.calibrationStatusMessage !== ""
                              ? root.calibrationStatusMessage
                              : (root.currentSummary !== "" ? root.currentSummary : qsTr("EDM Default (128 BPM) · Play or click Analyze Song"))
                        color: root.currentSummary !== "" || root.calibrationStatusMessage !== "" ? Theme.textBright : Theme.textSecondary
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }

                AppButton {
                    text: renderService.isRendering ? qsTr("Analyzing %1%").arg(Math.round(renderService.progress * 100)) : qsTr("Analyze Song")
                    implicitWidth: Math.max(Constants.defaultButtonWidth, implicitContentWidth + 16)
                    enabled: !renderService.isRendering
                    onClicked: {
                        root.calibrationStatusMessage = "";
                        renderService.calibrateMasterEffects();
                    }
                    ToolTip.visible: hovered
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.text: qsTr("Scan the full song off-line to lock in EDM EQ balance, multiband thresholds, and exact LUFS drive.")
                }

                AppButton {
                    text: qsTr("Reset Meters")
                    implicitWidth: Math.max(Constants.defaultButtonWidth, implicitContentWidth + 16)
                    onClicked: {
                        mmModel.resetMeters();
                        root.refreshTelemetry();
                    }
                }
            }

            // 4. 6 Side-by-Side Stage Cards
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                // Stage 1: Sub & Phase
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.subEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.subEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("Sub & Phase")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.subEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.subCleanEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Column {
                                anchors.centerIn: parent
                                width: parent.width - 8
                                spacing: 1
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: qsTr("24dB HPF")
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: root.subEnabled ? themeService.accentColor : Theme.textSecondary
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: qsTr("Mono Low")
                                    font.pixelSize: 10
                                    color: Theme.textSecondary
                                }
                            }
                        }
                        Label {
                            text: "HPF " + Math.round(root.currentSubHpfHz) + "Hz"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Mono " + Math.round(root.currentMonoBassHz) + "Hz"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Stage 2: Auto EQ
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.eqEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.eqEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("Auto EQ")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.eqEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.eqEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Rectangle {
                                anchors.top: parent.top
                                anchors.topMargin: 3
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 14
                                height: Math.min(parent.height - 6, (Math.abs(root.currentResCutDb) / 6.0) * (parent.height - 6))
                                color: themeService.accentColor
                            }
                        }
                        Label {
                            text: "EQ " + (root.currentAutoEqMoveDb >= 0 ? "+" : "") + root.currentAutoEqMoveDb.toFixed(1) + "dB"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Cut " + root.currentResCutDb.toFixed(1) + "dB"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Stage 3: Multiband
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.mbEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.mbEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("Multiband")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.mbEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.multibandEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Row {
                                anchors.centerIn: parent
                                height: parent.height - 6
                                spacing: 5
                                Repeater {
                                    model: [root.currentMbLowGrDb, root.currentMbMidGrDb, root.currentMbHighGrDb]
                                    Rectangle {
                                        width: 10
                                        height: parent.height
                                        color: Theme.surfacePanel
                                        Rectangle {
                                            anchors.top: parent.top
                                            width: parent.width
                                            height: Math.min(parent.height, (Math.abs(modelData) / 12.0) * parent.height)
                                            color: themeService.accentColor
                                        }
                                    }
                                }
                            }
                        }
                        Label {
                            text: "L " + Math.abs(root.currentMbLowGrDb).toFixed(1) + " M " + Math.abs(root.currentMbMidGrDb).toFixed(1)
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "H " + root.currentMbHighGrDb.toFixed(1) + "dB"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Stage 4: Stereo & Air
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.stereoEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.stereoEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("Stereo & Air")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.stereoEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.stereoEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Item {
                                anchors.fill: parent
                                anchors.margins: 6
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width
                                    height: 8
                                    color: Theme.surfacePanel
                                    border.color: Theme.borderSubtle
                                    Rectangle {
                                        width: 1
                                        height: parent.height
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        color: Theme.separatorStrong
                                    }
                                    Rectangle {
                                        y: 1
                                        height: parent.height - 2
                                        width: 6
                                        radius: 2
                                        x: Math.max(1, Math.min(parent.width - 7, ((root.currentStereoCorr + 1.0) * 0.5) * (parent.width - 6)))
                                        color: root.currentStereoCorr < 0 ? Theme.danger : themeService.accentColor
                                    }
                                }
                            }
                        }
                        Label {
                            text: "Corr " + (root.currentStereoCorr >= 0 ? "+" : "") + root.currentStereoCorr.toFixed(2)
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Width " + Math.round(root.currentStereoWidthPct) + "%"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Stage 5: Soft Clipper
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.clipperEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.clipperEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("Soft Clipper")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.clipperEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.clipperEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Rectangle {
                                anchors.top: parent.top
                                anchors.topMargin: 3
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 14
                                height: Math.min(parent.height - 6, (Math.abs(root.currentClipGrDb) / 12.0) * (parent.height - 6))
                                color: themeService.accentColor
                            }
                        }
                        Label {
                            text: "4x OS"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Clip " + root.currentClipGrDb.toFixed(1) + "dB"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Stage 6: True-Peak Limiter
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.preferredHeight: Math.max(122, Math.round(root.height * 0.23))
                    color: Theme.surfacePanel
                    border.color: root.limiterEnabled ? themeService.accentColor : Theme.borderSubtle
                    opacity: root.limiterEnabled ? 1.0 : 0.55
                    radius: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 2

                        Label {
                            text: qsTr("True-Peak")
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: root.limiterEnabled
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.limiterEnabled, checked ? 1 : 0)
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.surfaceSunken
                            border.color: Theme.borderSubtle
                            radius: 2
                            Rectangle {
                                anchors.top: parent.top
                                anchors.topMargin: 3
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 14
                                height: Math.min(parent.height - 6, (Math.abs(root.currentLimGrDb) / 18.0) * (parent.height - 6))
                                color: themeService.accentColor
                            }
                        }
                        Label {
                            text: "Ceil " + root.peakLimitDb.toFixed(1)
                            font.family: "Monospace"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "GR " + root.currentLimGrDb.toFixed(1) + "dB"
                            font.family: "Monospace"
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: root
        function onAboutToShow() {
            root.calibrationStatusMessage = "";
            root.refreshTelemetry();
        }
    }

    Connections {
        target: renderService
        function onCalibrationFinished(success, message) {
            if (message !== "") {
                root.calibrationStatusMessage = message;
            }
            if (success) {
                uiLogger.info("MagicMaster", message);
            } else {
                uiLogger.warning("MagicMaster", message);
            }
            root.refreshTelemetry();
        }
    }

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: root.refreshTelemetry()
    }

    MagicMasterDialogModel {
        id: mmModel
        rack: effectRackController
        effectIndex: root.effectIndex
    }
}
