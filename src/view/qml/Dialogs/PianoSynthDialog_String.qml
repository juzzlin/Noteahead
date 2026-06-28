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
        text: qsTr("String")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Brightness")
        value: pianoSynthController.brightness
        onMoved: v => pianoSynthController.brightness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        value: pianoSynthController.decay
        onMoved: v => pianoSynthController.decay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Inharmonicity")
        value: pianoSynthController.inharmonicity
        onMoved: v => pianoSynthController.inharmonicity = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Hammer Hardness")
        value: pianoSynthController.hammerHardness
        onMoved: v => pianoSynthController.hammerHardness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release Time")
        value: pianoSynthController.releaseTime
        onMoved: v => pianoSynthController.releaseTime = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("String Detune")
        value: pianoSynthController.stringDetune
        onMoved: v => pianoSynthController.stringDetune = v
        Layout.fillWidth: true
    }
}
