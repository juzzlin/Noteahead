// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import ".."

AnimatedDialog {
    id: rootItem
    title: qsTr("User Manual")
    modal: true
    visible: false

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

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
