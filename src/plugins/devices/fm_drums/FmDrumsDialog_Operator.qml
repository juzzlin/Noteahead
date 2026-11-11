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

//! One operator of the selected pad. The dialog repeats this over fmDrumsController.operators, as
//! the FM Synth repeats its own panel. Attack and decay only: a drum is struck rather than held, so
//! nothing here sustains and nothing is released.
ColumnLayout {
    id: root

    //! The FmDrumOperatorController this panel edits.
    required property var op

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 6

    RowLayout {
        Layout.fillWidth: true

        SectionHeading {
            text: root.op.title
        }

        Item {
            Layout.fillWidth: true
        }

        //! The same level knob means loudness on a carrier and brightness on a modulator
        Label {
            text: root.op.carrier ? qsTr("Carrier") : qsTr("Modulator")
            font.pixelSize: 12
            color: root.op.carrier ? themeService.accentColor : Theme.textMuted
        }
    }

    Label {
        text: root.op.modulatedBy.length ? qsTr("Modulated by") + " " + root.op.modulatedBy.join(", ") : qsTr("Not modulated")
        font.pixelSize: 11
        color: Theme.textMuted
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    ComboBox {
        model: fmDrumsController.operatorWaveformNames
        currentIndex: root.op.waveform
        onActivated: i => root.op.waveform = i
        Layout.fillWidth: true
    }

    GridLayout {
        columns: 2
        columnSpacing: 10
        rowSpacing: 6
        Layout.fillWidth: true

        // Any ratio, whole or not: the uneven ones are where the metal comes from
        Knob {
            label: qsTr("Ratio")
            mapping: "exponential"
            mapMin: 0.25
            mapMax: 16
            suffix: ""
            value: root.op.ratio
            onMoved: v => root.op.ratio = v
        }
        Knob {
            label: qsTr("Level")
            value: root.op.level
            onMoved: v => root.op.level = v
        }
        Knob {
            label: qsTr("Attack")
            mapping: "cubic"
            mapMin: 0
            mapMax: 1.0
            suffix: "s"
            value: root.op.attack
            onMoved: v => root.op.attack = v
        }
        Knob {
            label: qsTr("Decay")
            mapping: "exponential"
            mapMin: 0.005
            mapMax: 8.0
            suffix: "s"
            value: root.op.decay
            onMoved: v => root.op.decay = v
        }
        Knob {
            label: qsTr("Curve")
            value: root.op.curve
            onMoved: v => root.op.curve = v
        }
    }
}
