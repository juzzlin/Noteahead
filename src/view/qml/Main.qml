// This file is part of Cacophony.
// Copyright (C) 2023 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Cacophony 1.0
import "Editor"

ApplicationWindow {
    id: mainWindow
    property bool screenInit: false
    visible: true
    title: qsTr("Cacophony")
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&New...")
            }
            MenuSeparator {
            }
            Action {
                text: qsTr("&Quit")
                onTriggered: close()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
            }
        }
    }
    Universal.theme: Universal.Light
    EditorView {
        id: editorView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: menuBar.bottom
        anchors.bottom: parent.bottom
    }
    function setWindowSizeAndPosition() {
        const defaultWindowScale = Constants.defaultWindowScale;
        width = config.loadWindowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).width;
        width = Math.max(width, Constants.minWindowWidth);
        height = config.loadWindowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).height;
        height = Math.max(height, Constants.minWindowHeight);
        setX(mainWindow.screen.width / 2 - width / 2);
        setY(mainWindow.screen.height / 2 - height / 2);
    }
    Component.onCompleted: setWindowSizeAndPosition()
    onClosing: config.saveWindowSize(Qt.size(mainWindow.width, mainWindow.height))
}
