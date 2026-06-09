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
        text: qsTr("Oscillator 2")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: wavetableSynthController.octaveNames
        currentIndex: wavetableSynthController.octaveNames.length > 0 ? wavetableSynthController.osc2Octave + 2 : 0
        onActivated: i => wavetableSynthController.osc2Octave = i - 2
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Position")
        value: wavetableSynthController.osc2Pos
        onMoved: v => wavetableSynthController.osc2Pos = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -1200
        mapMax: 1200
        suffix: "c"
        value: wavetableSynthController.osc2Pitch
        onMoved: v => wavetableSynthController.osc2Pitch = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Level")
        value: wavetableSynthController.osc2Level
        onMoved: v => wavetableSynthController.osc2Level = v
        Layout.fillWidth: true
    }
}
