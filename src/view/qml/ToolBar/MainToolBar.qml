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

Rectangle {
    id: rootItem
    height: editorControlsContainer.height
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: themeService.mainToolBarGradientStartColor
        }
        GradientStop {
            position: 1.0
            color: themeService.mainToolBarGradientStopColor
        }
    }
    ScrollView {
        id: editorControlsContainer
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
        anchors.top: parent.top
        height: editorControlsWrapper.height
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        contentWidth: editorControls.width
        // Wrapper to allow vertical centering of content
        Item {
            id: editorControlsWrapper
            width: rootItem.width
            height: editorControls.height + 40
            anchors.verticalCenter: parent.verticalCenter
            EditorControls {
                id: editorControls
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
