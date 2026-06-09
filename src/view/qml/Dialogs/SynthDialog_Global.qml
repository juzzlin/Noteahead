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

    RowLayout {
        ComboBox {
            model: synthController.voiceModes
            currentIndex: synthController.voiceMode
            onActivated: i => synthController.voiceMode = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Voice Depth")
        value: synthController.voiceDepth
        onMoved: v => synthController.voiceDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Portamento")
        value: synthController.portamento
        onMoved: v => synthController.portamento = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: synthController.panSpread
        onMoved: v => synthController.panSpread = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Bend Range")
        from: 0
        to: 24
        stepSize: 1
        suffix: ""
        value: synthController.pitchBendRange
        onMoved: v => synthController.pitchBendRange = Math.round(v)
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Master Volume")
        mapping: "volume"
        value: synthController.volume
        onMoved: v => synthController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: synthController.gain
        onMoved: v => synthController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: synthController.pan
        onMoved: v => synthController.pan = v
        Layout.fillWidth: true
    }
}
