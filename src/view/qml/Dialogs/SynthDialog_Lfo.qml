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
        text: qsTr("LFO 1")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.lfoWaveformNames
            currentIndex: synthController.lfoWaveform
            onActivated: i => synthController.lfoWaveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: synthController.lfoModeNames
            currentIndex: synthController.lfoMode
            onActivated: i => synthController.lfoMode = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        ComboBox {
            model: synthController.lfoTargetNames
            currentIndex: synthController.lfoTarget
            onActivated: i => synthController.lfoTarget = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Rate")
        value: synthController.lfoRate
        onMoved: v => synthController.lfoRate = v
        Layout.fillWidth: true
        visible: synthController.lfoMode !== 1
    }
    SyncSlider {
        label: qsTr("Rate")
        value: synthController.lfoRate
        onMoved: v => synthController.lfoRate = v
        Layout.fillWidth: true
        visible: synthController.lfoMode === 1
    }
    Knob {
        label: qsTr("Intensity")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        value: synthController.lfoInt
        onMoved: v => synthController.lfoInt = v
        Layout.fillWidth: true
    }
}
