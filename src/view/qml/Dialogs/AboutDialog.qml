import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs

Dialog {
    id: rootItem
    title: qsTr("About")
    modal: true
    standardButtons: DialogButtonBox.Ok
    Column {
        anchors.centerIn: parent
        Label {
            text: `<strong>${applicationService.applicationName()} MIDI tracker v${applicationService.applicationVersion()}</strong>`
        }
        Label {
            text: " "
        }
        Label {
            text: `Licensed under ${applicationService.license()}`
        }
        Label {
            text: " "
        }
        Label {
            text: `${applicationService.copyright()}`
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
