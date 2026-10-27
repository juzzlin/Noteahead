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

//! One sweep: from Start to End over a length, in seconds or synced to the tempo. The dialog shows
//! one of these per sweep.
ColumnLayout {
    id: root

    //! The NoiseFxSweepController this panel edits.
    required property var sweep

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: root.sweep.index === 0 ? qsTr("Sweep A") : qsTr("Sweep B")
        Layout.topMargin: 10
    }
    ColumnLayout {
        spacing: 2
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Mode")
        }
        ComboBox {
            // In SweepEnvelope::Mode order
            model: {
                languageService.activeLanguage;
                return [qsTr("Hold"), qsTr("Loop"), qsTr("Ping-Pong")];
            }
            currentIndex: root.sweep.mode
            onActivated: i => root.sweep.mode = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Start")
        value: root.sweep.start
        onMoved: v => root.sweep.start = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("End")
        value: root.sweep.end
        onMoved: v => root.sweep.end = v
        Layout.fillWidth: true
    }
    Knob {
        // Above the middle it starts slowly and accelerates, below it moves quickly first
        label: qsTr("Curve")
        mapping: "bipolar"
        mapMin: -100
        mapMax: 100
        value: root.sweep.curve
        onMoved: v => root.sweep.curve = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Sync")
        checked: root.sweep.sync
        onToggled: root.sweep.sync = checked
    }
    ColumnLayout {
        spacing: 2
        visible: root.sweep.sync
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Length")
        }
        ComboBox {
            model: noiseFxController.syncLengthNames
            currentIndex: root.sweep.syncLength
            onActivated: i => root.sweep.syncLength = i
            Layout.fillWidth: true
        }
    }
    Knob {
        visible: !root.sweep.sync
        label: qsTr("Length")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 10
        mapMax: 60000
        value: root.sweep.length
        onMoved: v => root.sweep.length = v
        Layout.fillWidth: true
    }
    NoiseFxDialog_Route {
        target1: root.sweep.target1
        target2: root.sweep.target2
        depth1: root.sweep.depth1
        depth2: root.sweep.depth2
        onTarget1Activated: target => root.sweep.target1 = target
        onTarget2Activated: target => root.sweep.target2 = target
        onDepth1Moved: depth => root.sweep.depth1 = depth
        onDepth2Moved: depth => root.sweep.depth2 = depth
    }
}
