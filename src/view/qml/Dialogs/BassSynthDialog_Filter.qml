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
    title: qsTr("Filter")
    Layout.fillWidth: true
    
    RowLayout {
        anchors.fill: parent
        spacing: 15

        FilterKnob {
            label: qsTr("LPF Cutoff")
            controller: bassSynthController
            value: bassSynthController.lpfCutoff
            onMoved: v => bassSynthController.lpfCutoff = v
            Layout.fillWidth: true
        }
        
        Knob {
            label: qsTr("Resonance")
            value: bassSynthController.lpfResonance
            onMoved: v => bassSynthController.lpfResonance = v
            Layout.fillWidth: true
        }

        FilterKnob {
            label: qsTr("HPF Cutoff")
            controller: bassSynthController
            isHpf: true
            value: bassSynthController.hpfCutoff
            onMoved: v => bassSynthController.hpfCutoff = v
            Layout.fillWidth: true
        }

        Knob {
            label: qsTr("Env Mod")
            value: bassSynthController.envMod
            onMoved: v => bassSynthController.envMod = v
            Layout.fillWidth: true
        }

        Knob {
            label: qsTr("Decay")
            value: bassSynthController.decay
            onMoved: v => bassSynthController.decay = v
            Layout.fillWidth: true
        }
    }
}
