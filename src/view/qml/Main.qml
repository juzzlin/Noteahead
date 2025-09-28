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
import "MainMenu"

ApplicationWindow {
    id: mainWindow
    visible: true
    title: _getWindowTitle()
    menuBar: MainMenu {}
    footer: BottomBar {
        id: bottomBar
        height: menuBar.height
        width: parent.width
    }
    property bool screenInit: false
    property var _editorView
    property var _songView
    readonly property string _tag: "Main"
    Universal.theme: Universal.Dark
    MainToolBar {
        id: mainToolBar
        anchors.top: menuBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
    Item {
        id: songViewContainer
        height: 32
        anchors.top: mainToolBar.bottom
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
    }
    Component {
        id: songViewComponent
        SongView {
            id: songView
        }
    }
    Item {
        id: editorViewContainer
        anchors.top: songViewContainer.bottom
        anchors.topMargin: 20
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
    ShortcutsDialog {
        id: shortcutsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
        width: parent.width * Constants.defaultDialogScale
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
    MidiExportDialog {
        id: midiExportDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale * 0.5
        height: parent.height * Constants.defaultDialogScale * 0.5
    }
    RecentFilesDialog {
        id: recentFilesDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
        onFileSelected: {
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
        height: parent.height * Constants.defaultDialogScale
        width: parent.width * Constants.defaultDialogScale
        onAccepted: uiLogger.info(_tag, "Settings accepted")
        onRejected: uiLogger.info(_tag, "Settings rejected")
    }
    ColumnSettingsDialog {
        id: columnSettingsDialog
        anchors.centerIn: parent
        height: parent.height * Constants.defaultDialogScale
        width: parent.width * Constants.defaultDialogScale
        onAccepted: uiLogger.info(_tag, "Column settings accepted")
        onRejected: uiLogger.info(_tag, "Column settings rejected")
    }
    TrackSettingsDialog {
        id: trackSettingsDialog
        anchors.centerIn: parent
        height: parent.height * Constants.defaultDialogScale
        width: parent.width * Constants.defaultDialogScale
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
        width: parent.width * Constants.defaultDialogScale
        property int trackIndex
        property int columnIndex
        onAccepted: mixerService.setColumnVelocityScale(trackIndex, columnIndex, value())
    }
    IntegerInputDialog {
        id: lineDelayDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: editorService.setDelayOnCurrentLine(value())
        Component.onCompleted: {
            setMinValue(0);
            setMaxValue(editorService.ticksPerLine());
        }
    }
    IntegerInputDialog {
        id: trackVelocityScaleDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        property int trackIndex
        onAccepted: mixerService.setTrackVelocityScale(trackIndex, value())
    }
    InterpolationDialog {
        id: columnVelocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: editorService.requestLinearVelocityInterpolationOnColumn(startLine(), endLine(), startValue(), endValue(), usePercentages())
    }
    InterpolationDialog {
        id: trackVelocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: editorService.requestLinearVelocityInterpolationOnTrack(startLine(), endLine(), startValue(), endValue(), usePercentages())
    }
    InterpolationDialog {
        id: selectionVelocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: editorService.requestLinearVelocityInterpolationOnSelection(startLine(), endLine(), startValue(), endValue(), usePercentages())
    }
    AddMidiCcAutomationDialog {
        id: addMidiCcAutomationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: {
            const position = editorService.position;
            const automationId = automationService.addMidiCcAutomation(position.pattern, position.track, position.column, addMidiCcAutomationDialog.controller(), addMidiCcAutomationDialog.startLine(), addMidiCcAutomationDialog.endLine(), addMidiCcAutomationDialog.startValue(), addMidiCcAutomationDialog.endValue(), addMidiCcAutomationDialog.comment());
            if (addMidiCcAutomationDialog.cycles() > 0 && addMidiCcAutomationDialog.amplitude() > 0) {
                automationService.addMidiCcModulation(automationId, addMidiCcAutomationDialog.cycles(), addMidiCcAutomationDialog.amplitude(), addMidiCcAutomationDialog.inverted());
            }
            selectionService.clear();
            bottomBar.setStatusText(qsTr("MIDI CC automation added"));
        }
    }
    EditMidiCcAutomationsDialog {
        id: editMidiCcAutomationsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
        onAccepted: midiCcAutomationsModel.applyAll()
    }
    AddPitchBendAutomationDialog {
        id: addPitchBendAutomationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: {
            const position = editorService.position;
            automationService.addPitchBendAutomation(position.pattern, position.track, position.column, startLine(), endLine(), startValue(), endValue(), comment());
            selectionService.clear();
            bottomBar.setStatusText(qsTr("Pitch Bend automation added"));
        }
    }
    EditPitchBendAutomationsDialog {
        id: editPitchBendAutomationsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
        onAccepted: pitchBendAutomationsModel.applyAll()
    }
    DelayCalculatorDialog {
        id: delayCalculatorDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale * 0.75
        height: parent.height * 0.25
    }
    GainConverterDialog {
        id: gainConverterDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale * 0.75
        height: parent.height * 0.25
    }
    NoteFrequencyDialog {
        id: noteFrequencyDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    MainContextMenu {
        id: contextMenu
        width: parent.width * 0.25
    }
    function _getWindowTitle(): string {
        const nameAndVersion = `${applicationService.applicationName()} MIDI tracker v${applicationService.applicationVersion()}`;
        const currentFileName = (editorService.currentFileName ? " - " + editorService.currentFileName : "");
        return `${nameAndVersion}${currentFileName}`;
    }
    function _setWindowSizeAndPosition(): void {
        const defaultWindowScale = Constants.defaultWindowScale;
        width = settingsService.windowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).width;
        width = Math.max(width, Constants.minWindowWidth);
        height = settingsService.windowSize(Qt.size(mainWindow.screen.width * defaultWindowScale, mainWindow.screen.height * defaultWindowScale)).height;
        height = Math.max(height, Constants.minWindowHeight);
        setX(mainWindow.screen.width / 2 - width / 2);
        setY(mainWindow.screen.height / 2 - height / 2);
    }
    function _connectApplicationService(): void {
        applicationService.alertDialogRequested.connect(text => {
            errorDialog.errorMessage = text;
            errorDialog.open();
        });
        applicationService.midiExportDialogRequested.connect(midiExportDialog.open);
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
        UiService.shortcutsDialogRequested.connect(shortcutsDialog.open);
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
        UiService.columnSettingsDialogRequested.connect((trackIndex, columnIndex) => {
            columnSettingsDialog.setColumn(trackIndex, columnIndex);
            columnSettingsDialog.open();
        });
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
            columnVelocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
            columnVelocityInterpolationDialog.setStartLine(0);
            columnVelocityInterpolationDialog.setEndLine(editorService.currentLineCount - 1);
            columnVelocityInterpolationDialog.setStartValue(0);
            columnVelocityInterpolationDialog.setEndValue(100);
            columnVelocityInterpolationDialog.open();
        });
        UiService.selectionVelocityInterpolationDialogRequested.connect(() => {
            selectionVelocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
            selectionVelocityInterpolationDialog.setStartLine(selectionService.minLine());
            selectionVelocityInterpolationDialog.setEndLine(selectionService.maxLine());
            selectionVelocityInterpolationDialog.setStartValue(0);
            selectionVelocityInterpolationDialog.setEndValue(100);
            selectionVelocityInterpolationDialog.open();
        });
        UiService.trackVelocityInterpolationDialogRequested.connect(() => {
            trackVelocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
            trackVelocityInterpolationDialog.setStartLine(0);
            trackVelocityInterpolationDialog.setEndLine(editorService.currentLineCount - 1);
            trackVelocityInterpolationDialog.setStartValue(0);
            trackVelocityInterpolationDialog.setEndValue(100);
            trackVelocityInterpolationDialog.open();
        });
        UiService.lineDelayDialogRequested.connect(() => {
            lineDelayDialog.setValue(editorService.delayAtCurrentPosition());
            lineDelayDialog.open();
        });
        UiService.lineAddMidiCcAutomationDialogRequested.connect(() => {
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setStartLine(editorService.position.line);
            addMidiCcAutomationDialog.setEndLine(editorService.position.line);
            addMidiCcAutomationDialog.setStartValue(100);
            addMidiCcAutomationDialog.setEndValue(0);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.open();
        });
        UiService.columnAddMidiCcAutomationDialogRequested.connect(() => {
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setStartLine(0);
            addMidiCcAutomationDialog.setEndLine(editorService.currentLineCount - 1);
            addMidiCcAutomationDialog.setStartValue(0);
            addMidiCcAutomationDialog.setEndValue(100);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.open();
        });
        UiService.selectionAddMidiCcAutomationDialogRequested.connect(() => {
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setStartLine(selectionService.minLine());
            addMidiCcAutomationDialog.setEndLine(selectionService.maxLine());
            addMidiCcAutomationDialog.setStartValue(0);
            addMidiCcAutomationDialog.setEndValue(100);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.open();
        });
        UiService.editMidiCcAutomationsDialogRequested.connect(() => {
            midiCcAutomationsModel.requestMidiCcAutomations();
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations"));
            editMidiCcAutomationsDialog.open();
        });
        UiService.editMidiCcAutomationsDialogByLineRequested.connect(() => {
            const position = editorService.position;
            midiCcAutomationsModel.requestMidiCcAutomationsByLine(position.pattern, position.track, position.column, position.line);
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by line"));
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
        UiService.lineAddPitchBendAutomationDialogRequested.connect(() => {
            addPitchBendAutomationDialog.setTitle(qsTr("Add Pitch Bend automation"));
            addPitchBendAutomationDialog.setStartLine(editorService.position.line);
            addPitchBendAutomationDialog.setEndLine(editorService.position.line);
            addPitchBendAutomationDialog.setStartValue(0);
            addPitchBendAutomationDialog.setEndValue(100);
            addPitchBendAutomationDialog.setComment("");
            addPitchBendAutomationDialog.open();
        });
        UiService.columnAddPitchBendAutomationDialogRequested.connect(() => {
            addPitchBendAutomationDialog.setTitle(qsTr("Add Pitch Bend automation"));
            addPitchBendAutomationDialog.setStartLine(0);
            addPitchBendAutomationDialog.setEndLine(editorService.currentLineCount - 1);
            addPitchBendAutomationDialog.setStartValue(0);
            addPitchBendAutomationDialog.setEndValue(100);
            addPitchBendAutomationDialog.setComment("");
            addPitchBendAutomationDialog.open();
        });
        UiService.selectionAddPitchBendAutomationDialogRequested.connect(() => {
            addPitchBendAutomationDialog.setTitle(qsTr("Add Pitch Bend automation"));
            addPitchBendAutomationDialog.setStartLine(selectionService.minLine());
            addPitchBendAutomationDialog.setEndLine(selectionService.maxLine());
            addPitchBendAutomationDialog.setStartValue(0);
            addPitchBendAutomationDialog.setEndValue(100);
            addPitchBendAutomationDialog.setComment("");
            addPitchBendAutomationDialog.open();
        });
        UiService.editPitchBendAutomationsDialogRequested.connect(() => {
            pitchBendAutomationsModel.requestPitchBendAutomations();
            editPitchBendAutomationsDialog.setTitle(qsTr("Edit Pitch Bend automations"));
            editPitchBendAutomationsDialog.open();
        });
        UiService.editPitchBendAutomationsDialogByLineRequested.connect(() => {
            const position = editorService.position;
            pitchBendAutomationsModel.requestPitchBendAutomationsByLine(position.pattern, position.track, position.column, position.line);
            editPitchBendAutomationsDialog.setTitle(qsTr("Edit Pitch Bend automations by line"));
            editPitchBendAutomationsDialog.open();
        });
        UiService.editPitchBendAutomationsDialogByColumnRequested.connect(() => {
            const position = editorService.position;
            pitchBendAutomationsModel.requestPitchBendAutomationsByColumn(position.pattern, position.track, position.column);
            editPitchBendAutomationsDialog.setTitle(qsTr("Edit Pitch Bend automations by column"));
            editPitchBendAutomationsDialog.open();
        });
        UiService.editPitchBendAutomationsDialogByTrackRequested.connect(() => {
            const position = editorService.position;
            pitchBendAutomationsModel.requestPitchBendAutomationsByTrack(position.pattern, position.track);
            editPitchBendAutomationsDialog.setTitle(qsTr("Edit Pitch Bend automations by track"));
            editPitchBendAutomationsDialog.open();
        });
        UiService.editPitchBendAutomationsDialogByPatternRequested.connect(() => {
            const position = editorService.position;
            pitchBendAutomationsModel.requestPitchBendAutomationsByPattern(position.pattern);
            editPitchBendAutomationsDialog.setTitle(qsTr("Edit Pitch Bend automations by pattern"));
            editPitchBendAutomationsDialog.open();
        });
        UiService.delayCalculatorDialogRequested.connect(() => {
            delayCalculatorDialog.bpm = editorService.beatsPerMinute;
            delayCalculatorDialog.calculateDelay();
            delayCalculatorDialog.open();
        });
        UiService.gainConverterDialogRequested.connect(() => {
            gainConverterDialog.open();
        });
        UiService.noteFrequencyDialogRequested.connect(() => {
            noteFrequencyDialog.open();
        });
        UiService.quitRequested.connect(() => {
            settingsService.setWindowSize(Qt.size(mainWindow.width, mainWindow.height));
            applicationService.requestQuit();
        });
        UiService.contextMenuRequested.connect((x, y) => {
            contextMenu.x = x;
            contextMenu.y = y;
            contextMenu.open();
        });
    }
    function _connectServices(): void {
        _connectApplicationService();
        _connectEditorService();
        _connectUiService();
    }
    function _initialize(): void {
        _setWindowSizeAndPosition();
        _editorView = editorViewComponent.createObject(editorViewContainer, {
            "height": editorViewContainer.height,
            "width": editorViewContainer.width
        });
        _songView = songViewComponent.createObject(songViewContainer, {
            "height": songViewContainer.height,
            "width": songViewContainer.width
        });
        _connectServices();
    }
    function _resize(): void {
        _editorView.resize(editorViewContainer.width, editorViewContainer.height);
        _songView.width = songViewContainer.width;
        _songView.height = songViewContainer.height;
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
