import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

GridLayout {
    columns: 2
    columnSpacing: 20
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    // Amp EG
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        Label {
            text: qsTr("Amp Envelope")
            font.bold: true
            font.pixelSize: 16
            color: themeService.accentColor
            Layout.topMargin: 10
        }

        Knob {
            label: qsTr("Attack")
            mapping: "exponential"
            mapMin: 0.001
            mapMax: 10.0
            suffix: "s"
            value: wavetableSynthController.ampAttack
            onMoved: v => wavetableSynthController.ampAttack = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Decay")
            mapping: "exponential"
            mapMin: 0.01
            mapMax: 10.0
            suffix: "s"
            value: wavetableSynthController.ampDecay
            onMoved: v => wavetableSynthController.ampDecay = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Sustain")
            value: wavetableSynthController.ampSustain
            onMoved: v => wavetableSynthController.ampSustain = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Release")
            mapping: "exponential"
            mapMin: 0.01
            mapMax: 10.0
            suffix: "s"
            value: wavetableSynthController.ampRelease
            onMoved: v => wavetableSynthController.ampRelease = v
            Layout.fillWidth: true
        }
    }

    // Mod EG
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        Label {
            text: qsTr("Mod Envelope")
            font.bold: true
            font.pixelSize: 16
            color: themeService.accentColor
            Layout.topMargin: 10
        }

        Knob {
            label: qsTr("Attack")
            mapping: "exponential"
            mapMin: 0.001
            mapMax: 10.0
            suffix: "s"
            value: wavetableSynthController.modAttack
            onMoved: v => wavetableSynthController.modAttack = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Decay")
            mapping: "exponential"
            mapMin: 0.01
            mapMax: 10.0
            suffix: "s"
            value: wavetableSynthController.modDecay
            onMoved: v => wavetableSynthController.modDecay = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Intensity")
            mapping: "cubicCentered"
            mapMin: -100
            mapMax: 100
            value: wavetableSynthController.modInt
            onMoved: v => wavetableSynthController.modInt = v
            Layout.fillWidth: true
        }
        ComboBox {
            model: wavetableSynthController.modTargetNames
            currentIndex: wavetableSynthController.modTarget
            onActivated: i => wavetableSynthController.modTarget = i
            Layout.fillWidth: true
        }
    }
}
