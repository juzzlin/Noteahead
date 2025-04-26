// This file is part of Noteahead.
// Copyright (C) 2023 Jussi Lind <jussi.lind@iki.fi>
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

import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import Noteahead 1.0
import "Dialogs"
import "Editor"
import "ToolBar"

ApplicationWindow {
    id: mainWindow
    visible: true
    title: _getWindowTitle()
    menuBar: MainMenu {
    }
    footer: BottomBar {
        id: bottomBar
        height: menuBar.height
        width: parent.width
    }
    property bool screenInit: false
    property var _editorView
    readonly property string _tag: "Main"
    Universal.theme: Universal.Dark
    MainToolBar {
        id: mainToolBar
        height: menuBar.height * 4
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
    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }
    FileDialog {
        id: openDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Noteahead files") + ` (*${applicationService.fileFormatExtension()})`, qsTr("All files") + ` (*)`]
        onAccepted: {
            uiLogger.info(_tag, "File selected to open: " + selectedFile);
            applicationService.openProject(selectedFile);
        }
        onRejected: {
            uiLogger.info(_tag, "Open dialog was canceled.");
            applicationService.cancelOpenProject();
        }
    }
    Dialog {
        id: errorDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: parent.height * 0.5
        title: "<strong>" + qsTr("Error") + "</strong>"
        modal: true
        footer: DialogButtonBox {
            Button {
                text: qsTr("Ok")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
        property alias errorMessage: errorMessage.text
        Label {
            id: errorMessage
            anchors.centerIn: parent
        }
        Component.onCompleted: visible = false
    }
    EventSelectionDialog {
        id: eventSelectionDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        onAccepted: uiLogger.info(_tag, "Event selection dialog accepted")
        onRejected: uiLogger.info(_tag, "Event selection dialog rejected.")
    }
    FileDialog {
        id: saveAsDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Noteahead files") + ` (*${applicationService.fileFormatExtension()})`]
        onAccepted: {
            uiLogger.info(_tag, "File selected to save: " + selectedFile);
            applicationService.saveProjectAs(selectedFile);
        }
        onRejected: {
            uiLogger.info(_tag, "Save As dialog canceled.");
            applicationService.cancelSaveProjectAs();
        }
    }
    FileDialog {
        id: saveAsTemplateDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Noteahead files") + ` (*${applicationService.fileFormatExtension()})`]
        onAccepted: {
            uiLogger.info(_tag, "File selected to save: " + selectedFile);
            applicationService.saveProjectAsTemplate(selectedFile);
        }
        onRejected: uiLogger.info(_tag, "Save As Template dialog canceled.")
    }
    RecentFilesDialog {
        id: recentFilesDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: parent.height * 0.5
        onAccepted: {
            uiLogger.info(_tag, "Recent file accepted: " + selectedFile);
            applicationService.openRecentProject(selectedFile);
        }
        onRejected: {
            uiLogger.info(_tag, "Recent file dialog was canceled.");
            applicationService.cancelRecentFileDialog();
        }
    }
    SettingsDialog {
        id: settingsDialog
        anchors.centerIn: parent
        height: parent.height * 0.5
        width: parent.width * 0.5
        onAccepted: uiLogger.info(_tag, "Settings accepted")
        onRejected: uiLogger.info(_tag, "Settings rejected")
    }
    TrackSettingsDialog {
        id: trackSettingsDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        onAccepted: uiLogger.info(_tag, "Track settings accepted")
        onRejected: uiLogger.info(_tag, "Track settings rejected")
    }
    UnsavedChangesDialog {
        id: unsavedChangesDialog
        anchors.centerIn: parent
    }
    IntegerInputDialog {
        id: columnVelocityScaleDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        property int trackIndex
        property int columnIndex
        onAccepted: mixerService.setColumnVelocityScale(trackIndex, columnIndex, value())
    }
    IntegerInputDialog {
        id: lineDelayDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        onAccepted: editorService.setDelayOnCurrentLine(value())
        Component.onCompleted: {
            setMinValue(0);
            setMaxValue(editorService.ticksPerLine());
        }
    }
    IntegerInputDialog {
        id: trackVelocityScaleDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        property int trackIndex
        onAccepted: mixerService.setTrackVelocityScale(trackIndex, value())
    }
    InterpolationDialog {
        id: velocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        onAccepted: editorService.requestLinearVelocityInterpolation(startLine(), endLine(), startValue(), endValue())
    }
    AddMidiCcAutomationDialog {
        id: addMidiCcAutomationDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        onAccepted: {
            const position = editorService.position;
            automationService.addMidiCcAutomation(position.pattern, position.track, position.column, controller(), startLine(), endLine(), startValue(), endValue(), comment());
        }
    }
    EditMidiCcAutomationsDialog {
        id: editMidiCcAutomationsDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: parent.height * 0.5
        onAccepted: midiCcAutomationsModel.applyAll()
    }
    function _getWindowTitle(): string {
        const nameAndVersion = `${applicationService.applicationName()} MIDI tracker v${applicationService.applicationVersion()}`;
        const currentFileName = (editorService.currentFileName ? " - " + editorService.currentFileName : "");
        return `${nameAndVersion}${currentFileName}`;
    }
    function _setWindowSizeAndPosition(): void {
        const defaultWindowScale = Constants.defaultWindowScale;
        width = config.windowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).width;
        width = Math.max(width, Constants.minWindowWidth);
        height = config.windowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).height;
        height = Math.max(height, Constants.minWindowHeight);
        setX(mainWindow.screen.width / 2 - width / 2);
        setY(mainWindow.screen.height / 2 - height / 2);
    }
    function _connectApplicationService(): void {
        applicationService.openDialogRequested.connect(openDialog.open);
        applicationService.recentFilesDialogRequested.connect(recentFilesDialog.open);
        applicationService.saveAsDialogRequested.connect(saveAsDialog.open);
        applicationService.saveAsTemplateDialogRequested.connect(saveAsTemplateDialog.open);
        applicationService.statusTextRequested.connect(bottomBar.setStatusText);
        applicationService.unsavedChangesDialogRequested.connect(unsavedChangesDialog.open);
    }
    function _connectEditorService(): void {
        editorService.errorTextRequested.connect(errorText => {
                errorDialog.errorMessage = "ERROR!!: " + errorText;
                errorDialog.open();
            });
        editorService.statusTextRequested.connect(bottomBar.setStatusText);
        editorService.positionChanged.connect((newPosition, oldPosition) => {
                bottomBar.setPosition(newPosition);
            });
    }
    function _connectUiService(): void {
        UiService.aboutDialogRequested.connect(aboutDialog.open);
        UiService.eventSelectionDialogRequested.connect(() => {
                eventSelectionDialog.requestData();
                eventSelectionDialog.open();
            });
        UiService.focusOnEditorViewRequested.connect(() => {
                uiLogger.info(_tag, "Settings focus on editor view");
                _editorView.focus = true;
            });
        UiService.recentFilesDialogRequested.connect(recentFilesDialog.open);
        UiService.settingsDialogRequested.connect(settingsDialog.open);
        UiService.trackSettingsDialogRequested.connect(trackIndex => {
                trackSettingsDialog.setTrackIndex(trackIndex);
                trackSettingsDialog.open();
            });
        UiService.columnVelocityScaleDialogRequested.connect((trackIndex, columnIndex) => {
                columnVelocityScaleDialog.setTitle(qsTr("Set velocity scale for column ") + columnIndex);
                columnVelocityScaleDialog.setValue(mixerService.columnVelocityScale(trackIndex, columnIndex));
                columnVelocityScaleDialog.trackIndex = trackIndex;
                columnVelocityScaleDialog.columnIndex = columnIndex;
                columnVelocityScaleDialog.open();
            });
        UiService.trackVelocityScaleDialogRequested.connect(trackIndex => {
                trackVelocityScaleDialog.setTitle(qsTr("Set velocity scale for track ") + "'" + editorService.trackName(trackIndex) + "'");
                trackVelocityScaleDialog.setValue(mixerService.trackVelocityScale(trackIndex));
                trackVelocityScaleDialog.trackIndex = trackIndex;
                trackVelocityScaleDialog.open();
            });
        UiService.columnVelocityInterpolationDialogRequested.connect(() => {
                velocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
                velocityInterpolationDialog.setStartLine(0);
                velocityInterpolationDialog.setEndLine(editorService.currentLineCount - 1);
                velocityInterpolationDialog.setStartValue(0);
                velocityInterpolationDialog.setEndValue(100);
                velocityInterpolationDialog.open();
            });
        UiService.selectionVelocityInterpolationDialogRequested.connect(() => {
                velocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
                velocityInterpolationDialog.setStartLine(selectionService.minLine());
                velocityInterpolationDialog.setEndLine(selectionService.maxLine());
                velocityInterpolationDialog.setStartValue(0);
                velocityInterpolationDialog.setEndValue(100);
                velocityInterpolationDialog.open();
            });
        UiService.lineDelayDialogRequested.connect(() => {
                lineDelayDialog.setValue(editorService.delayAtCurrentPosition());
                lineDelayDialog.open();
            });
        UiService.columnAddMidiCcAutomationDialogRequested.connect(() => {
                addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
                addMidiCcAutomationDialog.setStartLine(0);
                addMidiCcAutomationDialog.setEndLine(editorService.currentLineCount - 1);
                addMidiCcAutomationDialog.setStartValue(0);
                addMidiCcAutomationDialog.setEndValue(100);
                addMidiCcAutomationDialog.setComment("");
                addMidiCcAutomationDialog.open();
            });
        UiService.selectionAddMidiCcAutomationDialogRequested.connect(() => {
                addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
                addMidiCcAutomationDialog.setStartLine(selectionService.minLine());
                addMidiCcAutomationDialog.setEndLine(selectionService.maxLine());
                addMidiCcAutomationDialog.setStartValue(0);
                addMidiCcAutomationDialog.setEndValue(100);
                addMidiCcAutomationDialog.setComment("");
                addMidiCcAutomationDialog.open();
            });
        UiService.editMidiCcAutomationsDialogRequested.connect(() => {
                midiCcAutomationsModel.requestMidiCcAutomations();
                editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations"));
                editMidiCcAutomationsDialog.open();
            });
        UiService.editMidiCcAutomationsDialogByColumnRequested.connect(() => {
                const position = editorService.position;
                midiCcAutomationsModel.requestMidiCcAutomationsByColumn(position.pattern, position.track, position.column);
                editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by column"));
                editMidiCcAutomationsDialog.open();
            });
        UiService.editMidiCcAutomationsDialogByTrackRequested.connect(() => {
                const position = editorService.position;
                midiCcAutomationsModel.requestMidiCcAutomationsByTrack(position.pattern, position.track);
                editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by track"));
                editMidiCcAutomationsDialog.open();
            });
        UiService.editMidiCcAutomationsDialogByPatternRequested.connect(() => {
                const position = editorService.position;
                midiCcAutomationsModel.requestMidiCcAutomationsByPattern(position.pattern);
                editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by pattern"));
                editMidiCcAutomationsDialog.open();
            });
        UiService.quitRequested.connect(() => {
                config.setWindowSize(Qt.size(mainWindow.width, mainWindow.height));
                applicationService.requestQuit();
            });
    }
    function _connectServices(): void {
        _connectApplicationService();
        _connectEditorService();
        _connectUiService();
    }
    function _initialize(): void {
        _setWindowSizeAndPosition();
        _editorView = editorViewComponent.createObject(contentArea, {
                "height": contentArea.height,
                "width": contentArea.width
            });
        _connectServices();
    }
    function _resize(): void {
        _editorView.resize(contentArea.width, contentArea.height);
    }
    Component.onCompleted: {
        _initialize();
    }
    onWidthChanged: _resize()
    onHeightChanged: _resize()
    onClosing: close => {
        UiService.requestQuit();
        close.accepted = false; // Let C++-side handle the termination
    }
}
