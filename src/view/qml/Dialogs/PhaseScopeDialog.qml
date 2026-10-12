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
    title: "<strong>" + qsTr("Phase Scope (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 1000
    height: parent ? parent.height * Constants.largeDialogScale : 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    function parameterValue(key: string): real {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, key);
    }
    function setParameterValue(key: string, value: real): void {
        effectRackController.setParameterValue(root.effectIndex, key, value);
    }

    // The scope is gated on the dialog being open, so a Phase Scope left in a rack costs nothing
    onVisibleChanged: {
        if (effectIndex >= 0) {
            effectRackController.phaseScopeSetActive(effectIndex, visible);
        }
    }

    property var reading: ({})

    Timer {
        interval: 33
        running: root.visible && root.effectIndex >= 0
        repeat: true
        onTriggered: {
            const capture = effectRackController.phaseScopeCapture(root.effectIndex, 512);
            scope.reference = capture.reference || [];
            scope.trace = capture.signal || [];
            root.reading = effectRackController.phaseScopeReading(root.effectIndex);
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 20

        ScrollView {
            id: controls
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            Layout.fillHeight: true
            Layout.preferredWidth: 260

            ColumnLayout {
                width: controls.availableWidth
                spacing: 8

                Label {
                    text: qsTr("Reference")
                    font.bold: true
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: {
                        // A JS block is not a translation binding: reading the language declares the dependency
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
                    currentIndex: root.parameterValue(effectRackController.phaseScopeReferenceKey()) + 1
                    onActivated: index => root.setParameterValue(effectRackController.phaseScopeReferenceKey(), index - 1)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("What the scope compares this signal with: usually the kick when this is the bass.")
                }

                Label {
                    text: qsTr("Polarity")
                    font.bold: true
                    Layout.topMargin: 6
                }
                RowLayout {
                    CheckBox {
                        text: qsTr("Invert L")
                        checked: root.parameterValue(effectRackController.phaseScopeInvertLeftKey()) > 0.5
                        onToggled: root.setParameterValue(effectRackController.phaseScopeInvertLeftKey(), checked ? 1 : 0)
                    }
                    CheckBox {
                        text: qsTr("Invert R")
                        checked: root.parameterValue(effectRackController.phaseScopeInvertRightKey()) > 0.5
                        onToggled: root.setParameterValue(effectRackController.phaseScopeInvertRightKey(), checked ? 1 : 0)
                    }
                }

                Label {
                    text: qsTr("Channels")
                    font.bold: true
                    Layout.topMargin: 6
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("Stereo"), qsTr("Left"), qsTr("Right"), qsTr("Swap"), qsTr("Mono")]
                    currentIndex: Math.round(root.parameterValue(effectRackController.phaseScopeChannelModeKey()))
                    onActivated: index => root.setParameterValue(effectRackController.phaseScopeChannelModeKey(), index)
                }

                Knob {
                    label: qsTr("Width")
                    // The number itself: it runs to 200 %, which the travel's percentage would not say
                    mapping: "value"
                    suffix: "%"
                    from: 0
                    to: 200
                    value: root.parameterValue(effectRackController.phaseScopeWidthKey()) * 200
                    onMoved: v => root.setParameterValue(effectRackController.phaseScopeWidthKey(), v / 200)
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                }
                CheckBox {
                    text: qsTr("Bass Mono")
                    checked: root.parameterValue(effectRackController.phaseScopeMonoBassKey()) > 0.5
                    onToggled: root.setParameterValue(effectRackController.phaseScopeMonoBassKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Both channels carry the same signal below the frequency, so the low end cannot cancel itself when played in mono.")
                }
                Knob {
                    label: qsTr("Bass Mono Frequency")
                    suffix: "Hz"
                    from: 20
                    to: 500
                    isInteger: true
                    enabled: root.parameterValue(effectRackController.phaseScopeMonoBassKey()) > 0.5
                    value: 20 + root.parameterValue(effectRackController.phaseScopeMonoBassFrequencyKey()) * 480
                    onMoved: v => root.setParameterValue(effectRackController.phaseScopeMonoBassFrequencyKey(), (v - 20) / 480)
                    Layout.fillWidth: true
                }

                Knob {
                    id: delayKnob
                    label: qsTr("Delay")
                    suffix: "ms"
                    from: 0
                    to: 20
                    isInteger: false
                    value: root.parameterValue(effectRackController.phaseScopeDelayKey()) * 20
                    onMoved: v => root.setParameterValue(effectRackController.phaseScopeDelayKey(), v / 20)
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                }
                RowLayout {
                    readonly property int samples: {
                        effectRackController.revision;
                        return effectRackController.phaseScopeDelaySamples(root.effectIndex);
                    }
                    id: delaySamples
                    AppButton {
                        text: "−1"
                        implicitWidth: 40
                        toolTipText: qsTr("One sample earlier")
                        onClicked: effectRackController.phaseScopeSetDelaySamples(root.effectIndex, delaySamples.samples - 1)
                    }
                    Label {
                        text: qsTr("%1 samples").arg(delaySamples.samples)
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    AppButton {
                        text: "+1"
                        implicitWidth: 40
                        toolTipText: qsTr("One sample later")
                        onClicked: effectRackController.phaseScopeSetDelaySamples(root.effectIndex, delaySamples.samples + 1)
                    }
                }

                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    from: -24
                    to: 24
                    isInteger: false
                    value: root.parameterValue(effectRackController.phaseScopeGainKey()) * 48 - 24
                    onMoved: v => root.setParameterValue(effectRackController.phaseScopeGainKey(), (v + 24) / 48)
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                }
                CheckBox {
                    text: qsTr("DC Block")
                    checked: root.parameterValue(effectRackController.phaseScopeDcBlockKey()) > 0.5
                    onToggled: root.setParameterValue(effectRackController.phaseScopeDcBlockKey(), checked ? 1 : 0)
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: "#333"
        }

        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                text: qsTr("Scope")
                font.bold: true
                font.pixelSize: 16
                color: themeService.accentColor
            }
            Label {
                text: root.reading.hasReference ? qsTr("Each hit of the reference, frozen: the reference in grey, this signal in colour. Where they swing the same way they add up; where they swing apart they cancel.") : qsTr("Choose a reference to compare this signal with. Without one the scope shows this signal alone.")
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: "#aaa"
                Layout.fillWidth: true
            }
            PhaseScopeRenderer {
                id: scope
                accentColor: themeService.accentColor
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 120
            }
            GridLayout {
                columns: 4
                columnSpacing: 20
                visible: root.reading.hasReference === true
                Label {
                    text: qsTr("Correlation")
                    font.bold: true
                }
                Label {
                    text: root.reading.correlation !== undefined ? root.reading.correlation.toFixed(2) : "–"
                }
                Label {
                    text: qsTr("Summed")
                    font.bold: true
                }
                Label {
                    // Against the two unrelated: +3 dB when they reinforce, 0 when unrelated, below 0 when they cancel
                    text: root.reading.sumGainDb !== undefined ? (root.reading.sumGainDb >= 0 ? "+" : "") + root.reading.sumGainDb.toFixed(1) + " dB" : "–"
                    color: root.reading.sumGainDb < -1 ? "#e06060" : (root.reading.sumGainDb > 1 ? themeService.accentColor : "white")
                }
                Label {
                    text: qsTr("Reference")
                    font.bold: true
                }
                Label {
                    text: root.reading.referenceDb !== undefined ? root.reading.referenceDb.toFixed(1) + " dB" : "–"
                }
                Label {
                    text: qsTr("Signal")
                    font.bold: true
                }
                Label {
                    text: root.reading.signalDb !== undefined ? root.reading.signalDb.toFixed(1) + " dB" : "–"
                }
            }
            Label {
                visible: root.reading.hasReference === true
                text: root.reading.correlation > 0.3 ? qsTr("Reinforcing: the two add up when they hit together.") : (root.reading.correlation < -0.3 ? qsTr("Cancelling: they take each other away. Try Invert, nudge the Delay, or an All-Pass Filter before this effect.") : qsTr("Unrelated: neither adding up nor cancelling much."))
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
