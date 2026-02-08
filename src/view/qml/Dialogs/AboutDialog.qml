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
            height: 256
            width: 256
            sourceSize: Qt.size(height, width)
            fillMode: Image.PreserveAspectFit
            source: "../Graphics/icon.png"
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
                text: `${qsTr("Project website:")} <a href="${applicationService.webSiteUrl()}">${applicationService.webSiteUrl()}</a>`
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
