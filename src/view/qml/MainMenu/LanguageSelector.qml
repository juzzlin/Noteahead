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
import ".."
import "../Components"

// Language picker for the right-hand end of the menu bar. Deliberately not a MainMenu_* entry: it
// is the one control whose label a user who cannot read the current language still has to find.
Item {
    id: rootItem
    implicitWidth: selectorRow.implicitWidth + 16
    implicitHeight: selectorRow.implicitHeight
    Row {
        id: selectorRow
        anchors.centerIn: parent
        spacing: 6
        Text {
            id: globeIcon
            // Not translated on purpose, like the language names themselves.
            text: "\u{1F310}"
            color: themeService.mainMenuTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: languageName
            text: languageService.nativeLanguageName(languageService.activeLanguage)
            color: themeService.mainMenuTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "▾"
            color: themeService.mainMenuTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        // The menu is opened from Main.qml through UiService: a Popup parented here, inside the
        // window's menu bar, sits outside the content item the overlay covers and never shows.
        onClicked: {
            const position = rootItem.mapToItem(null, 0, rootItem.height);
            UiService.requestLanguageMenu(position.x, position.y);
        }
    }
}
