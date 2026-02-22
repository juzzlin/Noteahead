import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0

Dialog {
    id: rootItem
    title: qsTr("User Manual")
    modal: true
    standardButtons: DialogButtonBox.Ok
    visible: false

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
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            color: "white"
            onLinkActivated: link => Qt.openUrlExternally(link)
            font.pointSize: 10
            leftPadding: 20
            rightPadding: 20
            topPadding: 20
            bottomPadding: 20
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            anchors.right: scrollView.right
        }
    }

    Component.onCompleted: {
        var manualUrl = Qt.resolvedUrl("../Manual.html");
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200 || xhr.status === 0) {
                    contentText.text = xhr.responseText;
                } else {
                    contentText.text = "Failed to load manual: " + xhr.status + " " + xhr.statusText;
                }
            }
        };
        xhr.open("GET", manualUrl);
        xhr.send();
    }
}
