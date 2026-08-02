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

GroupBox {
    title: qsTr("Oscillator")
    Layout.fillWidth: true
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        RowLayout {
            spacing: 20
            ColumnLayout {
                Label { text: qsTr("VCO Waveform") }
                ComboBox {
                    model: bassSynthController.vcoWaveformNames
                    currentIndex: bassSynthController.waveform
                    onActivated: i => bassSynthController.waveform = i
                    Layout.preferredWidth: 120
                }
            }
            
            Knob {
                label: qsTr("Tuning")
                mapping: "cubicCentered"
                mapMin: -1200
                mapMax: 1200
                suffix: "c"
                value: bassSynthController.tuning
                onMoved: v => bassSynthController.tuning = v
                Layout.preferredWidth: 100
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }

        RowLayout {
            spacing: 20
            ColumnLayout {
                Label { text: qsTr("Sub Octave") }
                ComboBox {
                    model: ["-1 Oct", "-2 Oct"]
                    currentIndex: bassSynthController.subOctave - 1
                    onActivated: i => bassSynthController.subOctave = i + 1
                    Layout.preferredWidth: 120
                }
            }

            Knob {
                label: qsTr("Sub Level")
                value: bassSynthController.subLevel
                onMoved: v => bassSynthController.subLevel = v
                Layout.preferredWidth: 100
            }
        }
    }
}
