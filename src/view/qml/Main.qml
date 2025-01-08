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

import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import Cacophony 1.0
import "Editor"
import "ToolBar"

ApplicationWindow {
    id: mainWindow
    visible: true
    title: `${applicationService.applicationName()} MIDI tracker v${applicationService.applicationVersion()}`
    menuBar: MainMenu {
    }
    property bool screenInit: false
    property var _editorView
    readonly property string _tag: "Main"
    Universal.theme: Universal.Dark
    MainToolBar {
        id: mainToolBar
        height: menuBar.height * 2
        anchors.top: menuBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
    Item {
        id: contentArea
        anchors.top: mainToolBar.bottom
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
    FileDialog {
        id: saveAsDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Cacophony files") + " (*.caco)"]
        onAccepted: {
            uiLogger.info(_tag, "File selected to save: " + selectedFile);
            applicationService.saveProjectAs(selectedFile);
        }
        onRejected: {
            uiLogger.info(_tag, "Save As dialog was canceled.");
            applicationService.cancelSaveProjectAs();
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
    function _connectApplicationService() {
        applicationService.saveAsDialogRequested.connect(() => {
            saveAsDialog.open();
        });
    }
    function _initialize() {
        _setWindowSizeAndPosition();
        _editorView = editorViewComponent.createObject(contentArea, {
            "height": contentArea.height,
            "width": contentArea.width
        });
        _connectApplicationService();
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
