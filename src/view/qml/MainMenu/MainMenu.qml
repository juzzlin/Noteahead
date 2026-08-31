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

// Wraps the MenuBar rather than being one, so that the language selector can sit at the right-hand
// end of the same bar. ApplicationWindow.menuBar accepts any Item, and the height is taken from the
// MenuBar so that everything sizing itself off menuBar.height still gets the bar's own height.
Item {
    id: rootItem
    implicitHeight: menuBar.implicitHeight
    MenuBar {
        id: menuBar
        // Fills the bar so its background paints the full width; the menus themselves are
        // left-aligned, which leaves the right-hand end free for the selector drawn on top.
        anchors.fill: parent
        MainMenu_File {}
        MainMenu_Edit {}
        MainMenu_Song {}
        MainMenu_Devices {}
        MainMenu_Effects {}
        MainMenu_Tools {}
        MainMenu_Help {}
    }
    LanguageSelector {
        id: languageSelector
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        z: 1
    }
}
