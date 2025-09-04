import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings") + "</strong>"
    modal: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                rootItem.accepted();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save current settings")
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10
        StackLayout {
            id: mainLayout
            height: parent.height - tabBar.height
            width: parent.width
            currentIndex: tabBar.currentIndex
            SettingsDialogGeneralSettings {
                height: parent.height
                width: parent.width
            }
            SettingsDialogMidiSettings {
                height: parent.height
                width: parent.width
            }
            SettingsDialogAudioSettings {
                height: parent.height
                width: parent.width
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("General")
            }
            TabButton {
                text: qsTr("MIDI")
            }
            TabButton {
                text: qsTr("Audio")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
