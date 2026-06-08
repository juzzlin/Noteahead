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
        text: qsTr("Filter")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: wavetableSynthController
        value: wavetableSynthController.lpfCutoff
        onMoved: v => wavetableSynthController.lpfCutoff = v
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("LPF Resonance")
        value: wavetableSynthController.lpfResonance
        onMoved: v => wavetableSynthController.lpfResonance = v
        Layout.fillWidth: true
    }

    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: wavetableSynthController
        value: wavetableSynthController.hpfCutoff
        onMoved: v => wavetableSynthController.hpfCutoff = v
        Layout.fillWidth: true
    }
}
