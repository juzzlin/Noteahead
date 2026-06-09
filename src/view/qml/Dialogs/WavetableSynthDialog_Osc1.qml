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
        text: qsTr("Oscillator 1")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: wavetableSynthController.octaveNames
        currentIndex: wavetableSynthController.osc1Octave + 2
        onActivated: i => wavetableSynthController.osc1Octave = i - 2
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Position")
        value: wavetableSynthController.osc1Pos
        onMoved: v => wavetableSynthController.osc1Pos = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -1200
        mapMax: 1200
        suffix: "c"
        value: wavetableSynthController.osc1Pitch
        onMoved: v => wavetableSynthController.osc1Pitch = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Level")
        value: wavetableSynthController.osc1Level
        onMoved: v => wavetableSynthController.osc1Level = v
        Layout.fillWidth: true
    }
}
