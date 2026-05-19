import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

GroupBox {
    title: qsTr("Distortion")
    Layout.fillWidth: true
    
    RowLayout {
        anchors.fill: parent
        spacing: 15

        Knob {
            label: qsTr("Drive")
            value: bassSynthController.distDrive
            onMoved: v => bassSynthController.distDrive = v
            Layout.fillWidth: true
        }

        Knob {
            label: qsTr("Tone")
            value: bassSynthController.distTone
            onMoved: v => bassSynthController.distTone = v
            Layout.fillWidth: true
        }

        Knob {
            label: qsTr("Level")
            value: bassSynthController.distLevel
            onMoved: v => bassSynthController.distLevel = v
            Layout.fillWidth: true
        }
    }
}
