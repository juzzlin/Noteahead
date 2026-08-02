import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

AnimatedDialog {
    id: rootItem
    title: `${qsTr("About ")} ${applicationService.applicationName()} ${qsTr("MIDI tracker v")}${applicationService.applicationVersion()}`
    modal: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
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
            Label {
                text: " "
            }
            Label {
                id: support_Text
                text: `${qsTr("Support this project by listening music created with Noteahead:")} <a href="https://www.arcticmusicproject.com">https://www.arcticmusicproject.com</a>.<br/>${qsTr("Spotify playlist:")} <a href="https://open.spotify.com/playlist/5yyhZlUsetq5C9NgPhAMQK">https://open.spotify.com/playlist/5yyhZlUsetq5C9NgPhAMQK</a>`
                onLinkActivated: link => Qt.openUrlExternally(link)
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 350
                MouseArea {
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
