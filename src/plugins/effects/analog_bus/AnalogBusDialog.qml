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
    title: "<strong>" + qsTr("Analog Bus (Slot %1)").arg(effectIndex + 1) + "</strong>"
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

    property real currentSaturationDb: 0.0

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 14

            EffectPresetRow {
                effectIndex: root.effectIndex
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Stereo console bus processor combining discrete A (American op-amp punch) and N (British LF transformer weight) saturation stages, a selectable Color slot, three-band Baxandall + Air Tone EQ, and elliptical Aspect widening.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.preferredWidth: 0
            }

            RowLayout {
                spacing: 16
                Layout.fillWidth: true

                GridLayout {
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 14
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    // Console Stages (A / N + Tight HPF)
                    SectionBox {
                        title: qsTr("Console")
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                spacing: 10
                                Layout.fillWidth: true

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Tight HPF")
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                    }
                                    ComboBox {
                                        model: [qsTr("Off"), qsTr("25 Hz (Mix)"), qsTr("50 Hz (Track)")]
                                        currentIndex: {
                                            effectRackController.revision;
                                            return Math.round(effectRackController.parameterValue(root.effectIndex, NahdXml.lowCutMode));
                                        }
                                        onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.lowCutMode, i)
                                        Layout.fillWidth: true
                                        ToolTip.visible: hovered
                                        ToolTip.delay: Constants.toolTipDelay
                                        ToolTip.text: qsTr("Subsonic tight filter: 25 Hz Mix adds a subtle low-end contour bump; 50 Hz Track is a flat Butterworth cut.")
                                    }
                                }

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Routing")
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                    }
                                    ComboBox {
                                        model: [qsTr("Off"), qsTr("A (Discrete)"), qsTr("N (Transformer)"), qsTr("A -> N"), qsTr("N -> A")]
                                        currentIndex: {
                                            effectRackController.revision;
                                            return Math.round(effectRackController.parameterValue(root.effectIndex, NahdXml.routing));
                                        }
                                        onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.routing, i)
                                        Layout.fillWidth: true
                                        ToolTip.visible: hovered
                                        ToolTip.delay: Constants.toolTipDelay
                                        ToolTip.text: qsTr("Order of discrete A and N console saturation stages.")
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 18
                                Layout.fillWidth: true

                                Knob {
                                    id: driveAKnob
                                    label: qsTr("A Drive")
                                    suffix: "dB"
                                    from: 0
                                    to: 36
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.driveA) * 36;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.driveA, v / 36)
                                    Layout.fillWidth: true
                                    ToolTip.visible: driveAKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Drive into Stage A: discrete American op-amp and steel transformer for punch and odd harmonics.")
                                }

                                Knob {
                                    id: driveNKnob
                                    label: qsTr("N Drive")
                                    suffix: "dB"
                                    from: 0
                                    to: 36
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.driveN) * 36;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.driveN, v / 36)
                                    Layout.fillWidth: true
                                    ToolTip.visible: driveNKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Drive into Stage N: British Class-A discrete circuit and iron transformer flux for even harmonics and low-end weight.")
                                }
                            }
                        }
                    }

                    // Expandable Color Slot
                    SectionBox {
                        title: qsTr("Color Module")
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                Label {
                                    text: qsTr("Module")
                                    font.pixelSize: 11
                                    color: Theme.textSecondary
                                }
                                ComboBox {
                                    model: [qsTr("Off"), qsTr("Bus 4000"), qsTr("Tape"), qsTr("Vinyl"), qsTr("Lo-Fi 12")]
                                    currentIndex: {
                                        effectRackController.revision;
                                        return Math.round(effectRackController.parameterValue(root.effectIndex, NahdXml.colorMode));
                                    }
                                    onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.colorMode, i)
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Series coloration module after the console stages: VCA bus glue, analog master tape, vinyl lathe warmth, or 12-bit sampler grit.")
                                }
                            }

                            RowLayout {
                                spacing: 18
                                Layout.fillWidth: true

                                Knob {
                                    id: colorDriveKnob
                                    label: qsTr("Color Drive")
                                    suffix: "%"
                                    from: 0
                                    to: 100
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.colorDrive) * 100;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.colorDrive, v / 100)
                                    Layout.fillWidth: true
                                    ToolTip.visible: colorDriveKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Intensity of the selected Color module.")
                                }

                                Knob {
                                    id: colorMixKnob
                                    label: qsTr("Color Mix")
                                    suffix: "%"
                                    from: 0
                                    to: 100
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.colorMix) * 100;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.colorMix, v / 100)
                                    Layout.fillWidth: true
                                    ToolTip.visible: colorMixKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Wet/dry blend inside the Color module slot.")
                                }
                            }
                        }
                    }

                    // Tone EQ Section
                    SectionBox {
                        title: qsTr("Tone EQ")
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                spacing: 14
                                Layout.fillWidth: true

                                CheckBox {
                                    text: qsTr("EQ In")
                                    checked: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.eqEnabled) > 0.5;
                                    }
                                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.eqEnabled, checked ? 1.0 : 0.0)
                                    ToolTip.visible: hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Engage or bypass the three-band Baxandall + Air Tone EQ.")
                                }

                                CheckBox {
                                    text: qsTr("Pre Saturation")
                                    checked: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.eqPre) > 0.5;
                                    }
                                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.eqPre, checked ? 1.0 : 0.0)
                                    ToolTip.visible: hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Route Tone EQ before the console and Color stages when checked, or after them when unchecked.")
                                }
                            }

                            RowLayout {
                                spacing: 10
                                Layout.fillWidth: true

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Low Band")
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                    }
                                    ComboBox {
                                        model: [qsTr("Sub 1 (30 Hz)"), qsTr("Sub 2 (45 Hz)"), qsTr("Bass (65 Hz)"), qsTr("Body (110 Hz)")]
                                        currentIndex: {
                                            effectRackController.revision;
                                            return Math.round(effectRackController.parameterValue(root.effectIndex, NahdXml.lowFreq));
                                        }
                                        onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.lowFreq, i)
                                        Layout.fillWidth: true
                                    }
                                }

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("High Band")
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                    }
                                    ComboBox {
                                        model: [qsTr("Mids (2.5 kHz)"), qsTr("Presence (4.5 kHz)"), qsTr("Brite (8 kHz)"), qsTr("Sheen (14 kHz)")]
                                        currentIndex: {
                                            effectRackController.revision;
                                            return Math.round(effectRackController.parameterValue(root.effectIndex, NahdXml.highFreq));
                                        }
                                        onActivated: i => effectRackController.setParameterValue(root.effectIndex, NahdXml.highFreq, i)
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 14
                                Layout.fillWidth: true

                                Knob {
                                    id: lowKnob
                                    label: qsTr("Low")
                                    suffix: "dB"
                                    from: -12
                                    to: 12
                                    value: {
                                        effectRackController.revision;
                                        return -12 + effectRackController.parameterValue(root.effectIndex, NahdXml.lowGain) * 24;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.lowGain, (v + 12) / 24)
                                    Layout.fillWidth: true
                                    ToolTip.visible: lowKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Gentle Baxandall low shelf (+/-12 dB).")
                                }

                                Knob {
                                    id: highKnob
                                    label: qsTr("High")
                                    suffix: "dB"
                                    from: -12
                                    to: 12
                                    value: {
                                        effectRackController.revision;
                                        return -12 + effectRackController.parameterValue(root.effectIndex, NahdXml.highGain) * 24;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.highGain, (v + 12) / 24)
                                    Layout.fillWidth: true
                                    ToolTip.visible: highKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Gentle Baxandall high shelf (+/-12 dB).")
                                }

                                Knob {
                                    id: airKnob
                                    label: qsTr("Air")
                                    suffix: "dB"
                                    from: -9
                                    to: 9
                                    value: {
                                        effectRackController.revision;
                                        return -9 + effectRackController.parameterValue(root.effectIndex, NahdXml.airGain) * 18;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.airGain, (v + 9) / 18)
                                    Layout.fillWidth: true
                                    ToolTip.visible: airKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Silky top-end air shelf (+/-9 dB).")
                                }
                            }
                        }
                    }

                    // Character & Master
                    SectionBox {
                        title: qsTr("Character & Master")
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                spacing: 14
                                Layout.fillWidth: true

                                CheckBox {
                                    text: qsTr("Vintage")
                                    checked: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.vintage) > 0.5;
                                    }
                                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.vintage, checked ? 1.0 : 0.0)
                                    ToolTip.visible: hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Gentle vintage transformer top-end roll-off to smooth ultrasonic bite.")
                                }

                                CheckBox {
                                    text: qsTr("Analog")
                                    checked: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.analog) > 0.5;
                                    }
                                    onToggled: effectRackController.setParameterValue(root.effectIndex, NahdXml.analog, checked ? 1.0 : 0.0)
                                    ToolTip.visible: hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Subtle deterministic L/R hardware tolerance and slow component drift.")
                                }
                            }

                            RowLayout {
                                spacing: 14
                                Layout.fillWidth: true

                                Knob {
                                    id: aspectKnob
                                    label: qsTr("Aspect")
                                    suffix: "%"
                                    from: 0
                                    to: 100
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.aspect) * 100;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.aspect, v / 100)
                                    Layout.fillWidth: true
                                    ToolTip.visible: aspectKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Elliptical high-passed Mid/Side width enhancement; keeps sub-bass focused and mono signals centered.")
                                }

                                Knob {
                                    id: outputKnob
                                    label: qsTr("Output")
                                    suffix: "dB"
                                    from: -12
                                    to: 12
                                    value: {
                                        effectRackController.revision;
                                        return -12 + effectRackController.parameterValue(root.effectIndex, NahdXml.gain) * 24;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.gain, (v + 12) / 24)
                                    Layout.fillWidth: true
                                    ToolTip.visible: outputKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Master output trim (+/-12 dB).")
                                }

                                Knob {
                                    id: mixKnob
                                    label: qsTr("Mix")
                                    suffix: "%"
                                    from: 0
                                    to: 100
                                    value: {
                                        effectRackController.revision;
                                        return effectRackController.parameterValue(root.effectIndex, NahdXml.mix) * 100;
                                    }
                                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.mix, v / 100)
                                    Layout.fillWidth: true
                                    ToolTip.visible: mixKnob.hovered
                                    ToolTip.delay: Constants.toolTipDelay
                                    ToolTip.text: qsTr("Parallel dry/wet blend with latency-matched dry signal.")
                                }
                            }
                        }
                    }
                }

                // Saturation meter
                ColumnLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: 66

                    Label {
                        text: qsTr("Saturation")
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        color: Theme.surfaceSunken
                        border.color: Theme.borderSubtle
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 260
                        Layout.minimumHeight: 260

                        Rectangle {
                            anchors.top: parent.top
                            anchors.topMargin: 2
                            width: parent.width - 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: Math.min(parent.height - 4, (Math.abs(root.currentSaturationDb) / 24.0) * (parent.height - 4))
                            color: themeService.accentColor
                        }

                        Item {
                            anchors.fill: parent
                            anchors.margins: 2
                            Repeater {
                                model: 7 // 0, -4, -8, -12, -16, -20, -24 dB
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: Theme.separatorStrong
                                    opacity: 0.5
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: (index / 6) * parent.height
                                }
                            }
                        }
                    }

                    Label {
                        text: root.currentSaturationDb.toFixed(1) + " dB"
                        color: themeService.accentColor
                        font.family: "Monospace"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentSaturationDb = dialogModel.saturationDb();
        }
    }

    AnalogBusDialogModel {
        id: dialogModel
        rack: effectRackController
        effectIndex: root.effectIndex
    }
}
