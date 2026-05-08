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
        text: qsTr("Mod EG (AD)")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: ["Pitch 1", "Pitch 2", "Cutoff"]
            currentIndex: synthController.modTarget
            onActivated: i => synthController.modTarget = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Attack")
        value: synthController.modAttack
        onMoved: v => synthController.modAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        value: synthController.modDecay
        onMoved: v => synthController.modDecay = v
        Layout.fillWidth: true
    }
    IntensityKnob {
        label: qsTr("Intensity")
        value: synthController.modInt
        onMoved: v => synthController.modInt = v
        Layout.fillWidth: true
    }
}
