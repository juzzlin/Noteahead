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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    Label {
        text: qsTr("Delay Effect")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: ["Stereo", "Mono", "PingPong", "Tape"]
            currentIndex: fmSynthController.delayType
            onActivated: i => fmSynthController.delayType = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        CheckBox {
            text: qsTr("Sync")
            checked: fmSynthController.delaySync
            onToggled: fmSynthController.delaySync = checked
        }
        StackLayout {
            currentIndex: fmSynthController.delaySync ? 0 : 1
            Layout.fillWidth: true
            Layout.preferredHeight: delayTimeKnob.implicitHeight
            SyncSlider {
                label: qsTr("Time")
                value: fmSynthController.delaySyncDivision
                onMoved: v => fmSynthController.delaySyncDivision = v
                Layout.fillWidth: true
            }
            Knob {
                id: delayTimeKnob
                label: qsTr("Time")
                mapping: "cubic"
                mapMin: 1
                mapMax: 10000
                to: 10000
                suffix: "ms"
                value: fmSynthController.delayTime
                onMoved: v => fmSynthController.delayTime = v
                Layout.fillWidth: true
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Feedback")
            value: fmSynthController.delayFeedback
            onMoved: v => fmSynthController.delayFeedback = v
            Layout.fillWidth: true
        }
        FilterKnob {
            label: qsTr("LPF")
            controller: fmSynthController
            value: fmSynthController.delayFeedbackLpf
            onMoved: v => fmSynthController.delayFeedbackLpf = v
            Layout.fillWidth: true
        }
        FilterKnob {
            label: qsTr("HPF")
            controller: fmSynthController
            value: fmSynthController.delayFeedbackHpf
            isHpf: true
            onMoved: v => fmSynthController.delayFeedbackHpf = v
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Depth")
            value: fmSynthController.delayDepth
            onMoved: v => fmSynthController.delayDepth = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Mix")
            value: fmSynthController.delayMix
            onMoved: v => fmSynthController.delayMix = v
            Layout.fillWidth: true
        }
    }

}
