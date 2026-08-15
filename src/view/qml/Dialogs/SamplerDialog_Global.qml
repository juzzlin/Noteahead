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
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 15

    // Global Settings
    Label {
        text: qsTr("Global Settings")
        font.bold: true
        color: themeService.accentColor
    }

    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: samplerController.gain
        onMoved: v => {
            samplerController.gain = v;
        }
    }

    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: samplerController.volume
        onMoved: v => {
            samplerController.volume = v;
        }
    }

    CheckBox {
        id: channelModeCheckbox
        text: qsTr("Map pads to MIDI channels 1-16 (for MIDI CC automation only)")
        Layout.fillWidth: true
        Layout.leftMargin: 20
        checked: samplerController.channelMode
        onToggled: samplerController.channelMode = checked
        contentItem: Label {
            text: channelModeCheckbox.text
            color: "white"
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            leftPadding: channelModeCheckbox.indicator.width + channelModeCheckbox.spacing
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("When enabled, MIDI CCs on channel 1 affect pad 1, channel 2 affects pad 2, and so on. Note playing is unaffected. Each pad also has its own Pan, Volume, LPF and HPF CCs that work regardless of this setting.")
    }

    CheckBox {
        id: embedCheckbox
        text: qsTr("Embed wave data in the project file")
        Layout.fillWidth: true
        Layout.leftMargin: 20
        checked: samplerController.embedWaveData
        onToggled: samplerController.embedWaveData = checked
        contentItem: Label {
            text: embedCheckbox.text
            color: "white"
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            leftPadding: embedCheckbox.indicator.width + embedCheckbox.spacing
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("When enabled, the sample data is saved directly into the project file. This makes the project file portable, but larger.")
    }
}
