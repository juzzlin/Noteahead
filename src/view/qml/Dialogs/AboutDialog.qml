import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    title: `${qsTr("About ")} ${applicationService.applicationName()} ${qsTr("MIDI tracker v")}${applicationService.applicationVersion()}`
    modal: true
    standardButtons: DialogButtonBox.Ok
    RowLayout {
        spacing: 10
        Image {
            id: icon
            fillMode: Image.PreserveAspectFit
            source: "../Graphics/icon.svg"
        }
        ColumnLayout {
            Label {
                text: `${qsTr("Licensed under")} ${applicationService.license()}`
            }
            Label {
                text: " "
            }
            Label {
                text: `${applicationService.copyright()}`
            }
            Label {
                text: " "
            }
            Label {
                id: link_Text
                text: `${qsTr("Project website:")} <a href="https://github.com/juzzlin/Noteahead">https://github.com/juzzlin/Noteahead</a>`
                onLinkActivated: link => Qt.openUrlExternally(link)
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
    Component.onCompleted: {
        visible = false;
    }
}
