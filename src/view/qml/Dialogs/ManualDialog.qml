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

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Text {
            id: contentText
            width: scrollView.availableWidth
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            color: "white"
            onLinkActivated: link => Qt.openUrlExternally(link)
            font.pointSize: 10
            leftPadding: 10
            rightPadding: 10
            topPadding: 10
            bottomPadding: 10
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
