import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Noteahead 1.0

ScrollView {
    clip: true
    property real moduleWidth: 0

    GridLayout {
        columns: 2
        columnSpacing: 20
        width: parent.width - 20

        WavetableSynthDialog_Filter {
            Layout.preferredWidth: moduleWidth * 1.5
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
        }

        GridLayout {
            columns: 2
            columnSpacing: 20
            Layout.preferredWidth: moduleWidth * 1.5
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            WavetableSynthDialog_AmpEg {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }

            WavetableSynthDialog_ModEg {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }
        }
    }
}
