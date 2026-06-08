import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

GroupBox {
    title: qsTr("Noise")
    font.bold: true
    font.pixelSize: 16
    Universal.accent: themeService.accentColor

    RowLayout {
        anchors.fill: parent
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
