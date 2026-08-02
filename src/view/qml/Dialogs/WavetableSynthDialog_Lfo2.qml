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
        text: qsTr("LFO 2")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: wavetableSynthController.lfo2WaveformNames
        currentIndex: wavetableSynthController.lfo2Waveform
        onActivated: i => wavetableSynthController.lfo2Waveform = i
        Layout.fillWidth: true
    }

    ComboBox {
        model: wavetableSynthController.lfo2ModeNames
        currentIndex: wavetableSynthController.lfo2Mode
        onActivated: i => wavetableSynthController.lfo2Mode = i
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Rate")
        value: wavetableSynthController.lfo2Rate
        onMoved: v => wavetableSynthController.lfo2Rate = v
        Layout.fillWidth: true
        visible: wavetableSynthController.lfo2Mode !== 1
    }

    SyncSlider {
        label: qsTr("Rate")
        value: wavetableSynthController.lfo2Rate
        onMoved: v => wavetableSynthController.lfo2Rate = v
        Layout.fillWidth: true
        visible: wavetableSynthController.lfo2Mode === 1
    }

    Knob {
        label: qsTr("Intensity")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        value: wavetableSynthController.lfo2Int
        onMoved: v => wavetableSynthController.lfo2Int = v
        Layout.fillWidth: true
    }

    ComboBox {
        model: wavetableSynthController.lfo2TargetNames
        currentIndex: wavetableSynthController.lfo2Target
        onActivated: i => wavetableSynthController.lfo2Target = i
        Layout.fillWidth: true
    }
}
