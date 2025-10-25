import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("&Tools")
    Action {
        text: qsTr("Delay time calculator")
        onTriggered: UiService.requestDelayCalculatorDialog()
    }
    Action {
        text: qsTr("Note frequencies")
        onTriggered: UiService.requestNoteFrequencyDialog()
    }
    Action {
        text: qsTr("Gain converter")
        onTriggered: UiService.requestGainConverterDialog()
    }
    delegate: MenuItemDelegate {}
}
