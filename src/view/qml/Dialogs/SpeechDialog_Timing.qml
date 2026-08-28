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
    spacing: 10

    Label {
        text: qsTr("Timing")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    ColumnLayout {
        Label { text: qsTr("Trigger") }
        ComboBox {
            model: [qsTr("Phrase"), qsTr("Step")]
            currentIndex: speechController.triggerMode
            onActivated: i => speechController.triggerMode = i
            Layout.fillWidth: true
        }
        Label {
            text: speechController.triggerMode === 0 ? qsTr("A note speaks the whole phrase.") : qsTr("A note speaks the next syllable.")
            color: "#999"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        Layout.topMargin: 10
        Label { text: qsTr("Sync") }
        ComboBox {
            model: [qsTr("Free"), qsTr("Fit"), qsTr("Grid")]
            currentIndex: speechController.syncMode
            onActivated: i => speechController.syncMode = i
            Layout.fillWidth: true
        }
        Label {
            text: speechController.syncMode === 0 ? qsTr("Natural durations, set by Rate.") : (speechController.syncMode === 1 ? qsTr("The phrase is stretched to span Length.") : qsTr("Each syllable takes one Division."))
            color: "#999"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    Knob {
        label: qsTr("Rate")
        enabled: speechController.syncMode === 0
        opacity: enabled ? 1.0 : 0.4
        value: speechController.rate
        onMoved: v => speechController.rate = v
        Layout.topMargin: 10
        Layout.fillWidth: true
    }

    ColumnLayout {
        Layout.topMargin: 10
        enabled: speechController.syncMode === 1
        opacity: enabled ? 1.0 : 0.4
        Label { text: qsTr("Length (1/16 steps)") }
        SpinBox {
            from: 1
            to: 64
            value: speechController.syncLength
            onValueModified: speechController.syncLength = value
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        Layout.topMargin: 10
        enabled: speechController.syncMode === 2
        opacity: enabled ? 1.0 : 0.4
        Label { text: qsTr("Division (1/16 steps)") }
        SpinBox {
            from: 1
            to: 16
            value: speechController.syncDivision
            onValueModified: speechController.syncDivision = value
            Layout.fillWidth: true
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
