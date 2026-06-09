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

    Label {
        text: qsTr("VCO 3")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.vcoWaveformNames
            currentIndex: synthController.vco3Waveform
            onActivated: i => synthController.vco3Waveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: synthController.octaveNames
            currentIndex: synthController.vco3Octave + 1
            onActivated: i => synthController.vco3Octave = i - 1
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -2400
        mapMax: 2400
        suffix: "c"
        value: synthController.vco3Pitch
        onMoved: v => synthController.vco3Pitch = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Shape")
        value: synthController.vco3Shape
        onMoved: v => synthController.vco3Shape = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.mixVco3
        onMoved: v => synthController.mixVco3 = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Hard Sync to VCO2")
        checked: synthController.vco3Sync
        onToggled: synthController.vco3Sync = checked
    }
}
