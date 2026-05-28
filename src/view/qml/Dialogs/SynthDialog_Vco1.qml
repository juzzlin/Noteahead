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
        text: qsTr("VCO 1")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.vcoWaveformNames
            currentIndex: synthController.vco1Waveform
            onActivated: i => synthController.vco1Waveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: ["16'", "8'", "4'", "2'"]
            currentIndex: synthController.vco1Octave + 1
            onActivated: i => synthController.vco1Octave = i - 1
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -2400
        mapMax: 2400
        suffix: "c"
        value: synthController.vco1Pitch
        onMoved: v => synthController.vco1Pitch = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Shape")
        value: synthController.vco1Shape
        onMoved: v => synthController.vco1Shape = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.mixVco1
        onMoved: v => synthController.mixVco1 = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Phase Sync")
        checked: synthController.vco1Sync
        onToggled: synthController.vco1Sync = checked
    }
}
