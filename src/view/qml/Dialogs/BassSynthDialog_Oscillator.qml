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
