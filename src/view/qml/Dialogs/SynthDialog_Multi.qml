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
        text: qsTr("Multi Engine")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.multiTypeNames
            currentIndex: synthController.multiType
            onActivated: i => synthController.multiType = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Shape")
        value: synthController.multiShape
        onMoved: v => synthController.multiShape = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Key Track")
        value: synthController.multiKeyTrack
        onMoved: v => synthController.multiKeyTrack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.multiLevel
        onMoved: v => synthController.multiLevel = v
        Layout.fillWidth: true
    }
}
