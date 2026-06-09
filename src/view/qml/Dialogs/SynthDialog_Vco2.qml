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
        text: qsTr("VCO 2")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.vcoWaveformNames
            currentIndex: synthController.vco2Waveform
            onActivated: i => synthController.vco2Waveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: synthController.octaveNames
            currentIndex: synthController.vco2Octave + 1
            onActivated: i => synthController.vco2Octave = i - 1
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -2400
        mapMax: 2400
        suffix: "c"
        value: synthController.vco2Pitch
        onMoved: v => synthController.vco2Pitch = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Shape")
        value: synthController.vco2Shape
        onMoved: v => synthController.vco2Shape = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.mixVco2
        onMoved: v => synthController.mixVco2 = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Hard Sync to VCO1")
        checked: synthController.vco2Sync
        onToggled: synthController.vco2Sync = checked
    }
}
