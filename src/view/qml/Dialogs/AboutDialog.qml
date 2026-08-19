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
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: `${qsTr("About ")} ${applicationService.applicationName()} ${applicationService.applicationVersion()}`
    modal: true
    footer: DialogButtonBox {
        AppButton {
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
                text: `${qsTr("Support this project by listening music created with Noteahead:")} <a href="https://www.arcticmusicproject.com">https://www.arcticmusicproject.com</a>.`
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
