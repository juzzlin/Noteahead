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

//! The time spent on the song, at the right-hand end of the menu bar: this session's, then the
//! song's total across every session. Dimmed while the timer waits for input. A click opens the
//! song's metadata, where the total can be reset.
Item {
    id: rootItem
    implicitWidth: timerRow.implicitWidth + 16
    implicitHeight: timerRow.implicitHeight

    //! Hours and minutes, as 1:05
    function _hoursMinutes(seconds: int): string {
        const minutes = Math.floor(seconds / 60);
        return Math.floor(minutes / 60) + ":" + String(minutes % 60).padStart(2, "0");
    }

    opacity: sessionTimerService.idle ? 0.5 : 1.0

    Row {
        id: timerRow
        anchors.centerIn: parent
        spacing: 6
        Text {
            // A stopwatch, not translated
            text: "⏱"
            color: themeService.mainMenuTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: rootItem._hoursMinutes(sessionTimerService.sessionSeconds) + " / " + rootItem._hoursMinutes(sessionTimerService.totalSeconds)
            color: themeService.mainMenuTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: UiService.requestSongMetadataDialog()
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: containsMouse
        ToolTip.text: (sessionTimerService.idle ? qsTr("Paused until the next input.") + " " : "") + qsTr("Time spent on this song: this session, then in all. Counting stops after %1 min without input or playback.").arg(settingsService.idleTimeoutMinutes)
    }
}
