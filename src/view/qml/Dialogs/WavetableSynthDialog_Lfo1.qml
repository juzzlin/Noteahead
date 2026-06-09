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
        text: qsTr("LFO 1")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: wavetableSynthController.lfoWaveformNames
        currentIndex: wavetableSynthController.lfoWaveform
        onActivated: i => wavetableSynthController.lfoWaveform = i
        Layout.fillWidth: true
    }

    ComboBox {
        model: wavetableSynthController.lfoModeNames
        currentIndex: wavetableSynthController.lfoMode
        onActivated: i => wavetableSynthController.lfoMode = i
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Rate")
        value: wavetableSynthController.lfoRate
        onMoved: v => wavetableSynthController.lfoRate = v
        Layout.fillWidth: true
        visible: wavetableSynthController.lfoMode !== 1
    }

    SyncSlider {
        label: qsTr("Rate")
        value: wavetableSynthController.lfoRate
        onMoved: v => wavetableSynthController.lfoRate = v
        Layout.fillWidth: true
        visible: wavetableSynthController.lfoMode === 1
    }

    Knob {
        label: qsTr("Intensity")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        value: wavetableSynthController.lfoInt
        onMoved: v => wavetableSynthController.lfoInt = v
        Layout.fillWidth: true
    }

    ComboBox {
        model: wavetableSynthController.lfoTargetNames
        currentIndex: wavetableSynthController.lfoTarget
        onActivated: i => wavetableSynthController.lfoTarget = i
        Layout.fillWidth: true
    }
}
