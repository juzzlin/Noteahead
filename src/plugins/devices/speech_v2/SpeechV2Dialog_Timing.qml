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

    SectionHeading {
        text: qsTr("Timing")
        Layout.topMargin: 10
    }

    ColumnLayout {
        Label { text: qsTr("Trigger") }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Phrase"), qsTr("Step"), qsTr("Line")];
            }
            currentIndex: speechV2Controller.triggerMode
            onActivated: i => speechV2Controller.triggerMode = i
            Layout.fillWidth: true
        }
        Label {
            text: speechV2Controller.triggerMode === 0 ? qsTr("A note speaks the whole phrase.") : (speechV2Controller.triggerMode === 1 ? qsTr("A note speaks the next syllable.") : qsTr("A note speaks the next line. A full stop, ! or ? starts a new one."))
            color: Theme.textTertiary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        Layout.topMargin: 10
        Label { text: qsTr("Sync") }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Free"), qsTr("Fit"), qsTr("Grid")];
            }
            currentIndex: speechV2Controller.syncMode
            onActivated: i => speechV2Controller.syncMode = i
            Layout.fillWidth: true
        }
        Label {
            text: speechV2Controller.syncMode === 0 ? qsTr("Natural durations, set by Rate.") : (speechV2Controller.syncMode === 1 ? (speechV2Controller.triggerMode === 2 ? qsTr("Each line fills the note that speaks it.") : qsTr("The phrase is stretched to span Length.")) : qsTr("Each syllable takes one Division."))
            color: Theme.textTertiary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    Knob {
        label: qsTr("Rate")
        enabled: speechV2Controller.syncMode === 0
        opacity: enabled ? 1.0 : 0.4
        value: speechV2Controller.rate
        onMoved: v => speechV2Controller.rate = v
        Layout.topMargin: 10
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Portamento")
        enabled: speechV2Controller.triggerMode !== 1
        opacity: enabled ? 1.0 : 0.4
        value: speechV2Controller.portamento
        onMoved: v => speechV2Controller.portamento = v
        Layout.topMargin: 10
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Hold a note on one column and play the melody on another: the second column moves the pitch without starting the next line.")
        color: Theme.textTertiary
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    ColumnLayout {
        Layout.topMargin: 10
        enabled: speechV2Controller.syncMode === 1 && speechV2Controller.triggerMode !== 2
        opacity: enabled ? 1.0 : 0.4
        Label { text: qsTr("Length (1/16 steps)") }
        SpinBox {
            from: 1
            to: 64
            value: speechV2Controller.syncLength
            onValueModified: speechV2Controller.syncLength = value
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        Layout.topMargin: 10
        enabled: speechV2Controller.syncMode === 2
        opacity: enabled ? 1.0 : 0.4
        Label { text: qsTr("Division (1/16 steps)") }
        SpinBox {
            from: 1
            to: 16
            value: speechV2Controller.syncDivision
            onValueModified: speechV2Controller.syncDivision = value
            Layout.fillWidth: true
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
