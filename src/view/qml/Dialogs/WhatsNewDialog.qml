import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import ".."

AnimatedDialog {
    id: rootItem
    title: qsTr("What's New")
    modal: true
    visible: false

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "white"
        // Light theme so the scroll bar is drawn for a light background rather than the dark one
        // the rest of the application uses
        Universal.theme: Universal.Light

        Flickable {
            id: scrollView
            anchors.fill: parent
            clip: true
            contentHeight: contentText.height
            contentWidth: width
            interactive: true
            boundsBehavior: Flickable.StopAtBounds

            Text {
                id: contentText
                width: scrollView.width
                // The CHANGELOG carries its meaning in its indentation, so it is shown as it is written
                textFormat: Text.PlainText
                font.family: "Monospace"
                font.pointSize: 10
                wrapMode: Text.WordWrap
                color: "black"
                leftPadding: 20
                rightPadding: 20
                topPadding: 20
                bottomPadding: 20
                text: applicationService.changeLog() !== "" ? applicationService.changeLog() : qsTr("Nothing has been released yet.")
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                anchors.right: scrollView.right
            }
        }
    }

    onOpened: scrollView.contentY = 0
}
