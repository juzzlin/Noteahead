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
        text: qsTr("Amp EG (ADSR)")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Attack")
        value: synthController.ampAttack
        onMoved: v => synthController.ampAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        value: synthController.ampDecay
        onMoved: v => synthController.ampDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: synthController.ampSustain
        onMoved: v => synthController.ampSustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release")
        value: synthController.ampRelease
        onMoved: v => synthController.ampRelease = v
        Layout.fillWidth: true
    }
}
