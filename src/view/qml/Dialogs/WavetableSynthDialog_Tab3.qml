import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0

ScrollView {
    clip: true
    property real moduleWidth: 0

    RowLayout {
        width: parent.width - 20
        spacing: 20
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor

        WavetableSynthDialog_Lfo1 {
            Layout.preferredWidth: moduleWidth * 1.5
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
        }

        WavetableSynthDialog_Lfo2 {
            Layout.preferredWidth: moduleWidth * 1.5
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
        }
    }
}
