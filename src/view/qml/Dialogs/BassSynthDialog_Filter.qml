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
