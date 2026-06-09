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
        text: qsTr("Voice / Global")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: wavetableSynthController.voiceModes
        currentIndex: wavetableSynthController.voiceMode
        onActivated: i => wavetableSynthController.voiceMode = i
        Layout.fillWidth: true
        Layout.bottomMargin: 10
    }
    ComboBox {
        model: wavetableSynthController.wavetableNames
        currentIndex: wavetableSynthController.wavetableIndex
        onActivated: i => wavetableSynthController.wavetableIndex = i
        Layout.fillWidth: true
        Layout.bottomMargin: 10
    }
    Knob {
        label: qsTr("Voice Depth")
        value: wavetableSynthController.voiceDepth
        onMoved: v => wavetableSynthController.voiceDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Portamento")
        value: wavetableSynthController.portamento
        onMoved: v => wavetableSynthController.portamento = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: wavetableSynthController.panSpread
        onMoved: v => wavetableSynthController.panSpread = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Bend Range")
        from: 0
        to: 24
        stepSize: 1
        suffix: ""
        value: wavetableSynthController.pitchBendRange
        onMoved: v => wavetableSynthController.pitchBendRange = Math.round(v)
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Master Volume")
        mapping: "volume"
        value: wavetableSynthController.volume
        onMoved: v => wavetableSynthController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: wavetableSynthController.gain
        onMoved: v => wavetableSynthController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: wavetableSynthController.pan
        onMoved: v => wavetableSynthController.pan = v
        Layout.fillWidth: true
    }
}
