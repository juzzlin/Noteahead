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
import ".."

//! That a newer Noteahead is out, at the right-hand end of the menu bar: a click opens the release's
//! page, the cross hides the notice until a newer version still. Takes no room while there is
//! nothing to tell.
Item {
    id: rootItem
    visible: updateService.availableVersion !== ""
    implicitWidth: visible ? noticeRow.implicitWidth + 16 : 0
    implicitHeight: noticeRow.implicitHeight

    Row {
        id: noticeRow
        anchors.centerIn: parent
        spacing: 8
        Text {
            id: noticeText
            text: qsTr("Noteahead %1 is available").arg(updateService.availableVersion)
            color: themeService.accentColor
            font.underline: openArea.containsMouse
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                id: openArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally(updateService.availableUrl)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: containsMouse
                ToolTip.text: qsTr("Open the release page")
            }
        }
        Text {
            // A cross, not translated
            text: "✕"
            color: dismissArea.containsMouse ? themeService.mainMenuTextColor : Qt.darker(themeService.mainMenuTextColor, 1.5)
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                id: dismissArea
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: updateService.dismiss()
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: containsMouse
                ToolTip.text: qsTr("Hide until a newer version comes out")
            }
        }
    }
}
