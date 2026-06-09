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
        text: qsTr("Noise")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Knob {
            label: qsTr("Noise Level")
            value: wavetableSynthController.noiseLevel
            onMoved: v => wavetableSynthController.noiseLevel = v
            Layout.fillWidth: true
        }
        Item { Layout.fillWidth: true }
        Item { Layout.fillWidth: true }
    }
}
