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
    title: "<strong>" + qsTr("Bit Crusher (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            Label {
                text: qsTr("Throws away word length and sample rate, separately. Fewer bits is grit that follows the signal; a lower rate folds everything above its Nyquist back down as aliases, which is the metallic half of the sound. Neither is oversampled, because here the aliases are the point.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            EffectPresetRow {
                effectIndex: root.effectIndex
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                // Discrete, so the knob carries the word length itself rather than a position on a
                // scale: Parameter keeps a discrete value raw where a continuous one is normalised.
                Knob {
                    label: qsTr("Bit Depth")
                    mapping: "integer"
                    suffix: " bits"
                    from: 1
                    to: effectRackController.bitCrusherMaxBits()
                    stepSize: 1
                    value: {
                        effectRackController.revision;
                        return Math.max(1, effectRackController.parameterValue(root.effectIndex, effectRackController.bitCrusherBitDepthKey()));
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bitCrusherBitDepthKey(), Math.round(v))
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Word length. The quantisation error follows the signal, which is the grit riding on top of it")
                }

                Knob {
                    label: qsTr("Rate")
                    suffix: " Hz"
                    mapping: "exponential"
                    mapMin: 100
                    mapMax: 48000
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.bitCrusherRateKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bitCrusherRateKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Samples are held at this rate. Everything above its Nyquist folds back as aliases, which is the metallic half of the sound")
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.bitCrusherMixKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.bitCrusherMixKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much of the crushed signal is heard against the clean one")
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
