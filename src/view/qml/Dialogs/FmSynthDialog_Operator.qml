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

//! One operator panel. The dialog repeats this over fmSynthController.operators rather than
//! carrying four copies of it: the operators are identical, and only the routing tells them apart.
ColumnLayout {
    id: root

    //! The FmOperatorController this panel edits.
    required property var op

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 10

        Label {
            text: root.op.title
            font.bold: true
            font.pixelSize: 16
            color: themeService.accentColor
        }

        Item {
            Layout.fillWidth: true
        }

        //! The same level knob means loudness on a carrier and brightness on a modulator, so the
        //! panel says which this operator currently is. It follows the algorithm, not the patch.
        Label {
            text: root.op.carrier ? qsTr("Carrier") : qsTr("Modulator")
            font.pixelSize: 12
            color: root.op.carrier ? themeService.accentColor : "#888"
        }
    }

    Label {
        text: root.op.modulatedBy.length ? qsTr("Modulated by") + " " + root.op.modulatedBy.join(", ") : qsTr("Not modulated")
        font.pixelSize: 11
        color: "#888"
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    ComboBox {
        model: fmSynthController.operatorWaveformNames
        currentIndex: root.op.waveform
        onActivated: i => root.op.waveform = i
        Layout.fillWidth: true
    }

    RowLayout {
        Layout.fillWidth: true

        Label {
            text: qsTr("Ratio")
        }
        ComboBox {
            model: fmSynthController.ratioNames
            currentIndex: root.op.ratio
            onActivated: i => root.op.ratio = i
            Layout.fillWidth: true
        }
    }

    Knob {
        label: qsTr("Detune")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        suffix: "c"
        value: root.op.detune
        onMoved: v => root.op.detune = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Level")
        value: root.op.level
        onMoved: v => root.op.level = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Velocity Sensitivity")
        value: root.op.velocitySensitivity
        onMoved: v => root.op.velocitySensitivity = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Key Scale")
        value: root.op.keyScale
        onMoved: v => root.op.keyScale = v
        Layout.fillWidth: true
    }

    //! Attack, decay and sustain only. An operator is not released with the note -- it holds where
    //! it is and the Amp EG fades the whole voice -- so there is no release here to set.
    Label {
        text: qsTr("Envelope")
        font.pixelSize: 12
        color: "#888"
        Layout.topMargin: 6
    }

    Knob {
        label: qsTr("Attack")
        mapping: "exponential"
        mapMin: 0.001
        mapMax: 10.0
        suffix: "s"
        value: root.op.attack
        onMoved: v => root.op.attack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.01
        mapMax: 10.0
        suffix: "s"
        value: root.op.decay
        onMoved: v => root.op.decay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: root.op.sustain
        onMoved: v => root.op.sustain = v
        Layout.fillWidth: true
    }
}
