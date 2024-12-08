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
    property bool screenInit: false
    property var _editorView
    Universal.theme: Universal.Dark
    Item {
        id: contentArea
        anchors.top: menuBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
    Component {
        id: editorViewComponent
        EditorView {
            id: editorView
        }
    }
    function _setWindowSizeAndPosition() {
        const defaultWindowScale = Constants.defaultWindowScale;
        width = config.loadWindowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).width;
        width = Math.max(width, Constants.minWindowWidth);
        height = config.loadWindowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).height;
        height = Math.max(height, Constants.minWindowHeight);
        setX(mainWindow.screen.width / 2 - width / 2);
        setY(mainWindow.screen.height / 2 - height / 2);
    }
    function _initialize() {
        _setWindowSizeAndPosition();
        _editorView = editorViewComponent.createObject(contentArea, {
                "height": contentArea.height,
                "width": contentArea.width
            });
    }
    function _resize() {
        _editorView.resize(contentArea.width, contentArea.height);
    }
    Component.onCompleted: {
        _initialize();
    }
    onWidthChanged: _resize()
    onHeightChanged: _resize()
    onClosing: config.saveWindowSize(Qt.size(mainWindow.width, mainWindow.height))
}
