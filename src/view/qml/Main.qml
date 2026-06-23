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
import QtQuick.Layouts
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
    property var _deviceNamesBeforeRackOpen: null
    property string _newDeviceName: ""
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
        anchors.bottom: audioWaveView.top
        anchors.left: parent.left
        anchors.right: parent.right
        onHeightChanged: _resize()
        onWidthChanged: _resize()
    }
    Component {
        id: editorViewComponent
        EditorView {
            id: editorView
        }
    }
    AudioWaveView {
        id: audioWaveView
        height: settingsService.waveViewEnabled ? bottomBar.height : 0
        visible: settingsService.waveViewEnabled
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
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
    ErrorDialog {
        id: errorDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    EventSelectionDialog {
        id: eventSelectionDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: uiLogger.info(_tag, "Event selection dialog accepted")
        onRejected: uiLogger.info(_tag, "Event selection dialog rejected.")
    }
    EffectsGalleryDialog {
        id: effectsGalleryDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    DeviceGalleryDialog {
        id: deviceGalleryDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
        onRejected: {
            uiLogger.info(_tag, "Save As Template dialog canceled.")
        }
    }
    FileDialog {
        id: exportDeviceSettingsDialog
        title: qsTr("Export Device Settings")
        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: [qsTr("Noteahead Device Settings") + ` (*${applicationService.deviceSettingsExtension()})`]
        property int slotIndex: -1
        onAccepted: {
            deviceRackController.exportSettings(slotIndex, selectedFile);
        }
    }
    FileDialog {
        id: importDeviceSettingsDialog
        title: qsTr("Import Device Settings")
        fileMode: FileDialog.OpenFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: [qsTr("Noteahead Device Settings") + ` (*${applicationService.deviceSettingsExtension()})`]
        property int slotIndex: -1
        onAccepted: {
            deviceRackController.importSettings(slotIndex, selectedFile);
        }
    }
    ConfirmationDialog {
        id: importDeviceSettingsConfirmationDialog
        anchors.centerIn: parent
        title: "<strong>" + qsTr("Device type mismatch") + "</strong>"
        acceptButtonText: qsTr("Replace")
        property int slotIndex: -1
        property url fileUrl
        onAccepted: deviceRackController.confirmImportSettings(slotIndex, fileUrl)
    }
    FileDialog {
        id: exportEffectSettingsDialog
        title: qsTr("Export Effect Settings")
        fileMode: FileDialog.SaveFile
        currentFolder: applicationService.lastEffectExportDirectory() !== "" ? applicationService.fromLocalFile(applicationService.lastEffectExportDirectory()) : StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: [qsTr("Noteahead Effect Settings (%1)").arg("*"+applicationService.effectRackSettingsExtension())]
        property int slotIndex: -1
        onAccepted: {
            applicationService.setLastEffectExportDirectory(exportEffectSettingsDialog.selectedFile);
            effectRackController.exportEffectSettings(slotIndex, exportEffectSettingsDialog.selectedFile);
        }
    }
    FileDialog {
        id: importEffectSettingsDialog
        title: qsTr("Import Effect Settings")
        fileMode: FileDialog.OpenFile
        currentFolder: applicationService.lastEffectImportDirectory() !== "" ? applicationService.fromLocalFile(applicationService.lastEffectImportDirectory()) : StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: [qsTr("Noteahead Effect Settings (%1)").arg("*"+applicationService.effectRackSettingsExtension())]
        property int slotIndex: -1
        onAccepted: {
            applicationService.setLastEffectImportDirectory(importEffectSettingsDialog.selectedFile);
            effectRackController.importEffectSettings(slotIndex, importEffectSettingsDialog.selectedFile);
        }
    }
    ConfirmationDialog {
        id: importEffectSettingsConfirmationDialog
        anchors.centerIn: parent
        title: "<strong>" + qsTr("Effect type mismatch") + "</strong>"
        acceptButtonText: qsTr("Replace")
        property int slotIndex: -1
        property url fileUrl
        onAccepted: effectRackController.confirmImportEffectSettings(slotIndex, fileUrl)
    }
    ConfirmationDialog {
        id: portAutoAssignDialog
        anchors.centerIn: parent
        title: "<strong>" + qsTr("Assign Instrument Port") + "</strong>"
        acceptButtonText: qsTr("Assign")
        onAccepted: trackSettingsDialog.setPortName(_newDeviceName)
    }
    MidiExportDialog {
        id: midiExportDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    AudioRenderDialog {
        id: audioRenderDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    MidiImportDialog {
        id: midiImportDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
    SamplerDialog {
        id: samplerDialog
        anchors.centerIn: parent
        width: parent.width * Constants.largeDialogScale
        height: parent.height * Constants.largeDialogScale
    }
    SynthDialog {
        id: synthDialog
        anchors.centerIn: parent
        width: parent.width * Constants.largeDialogScale
        height: parent.height * Constants.largeDialogScale
    }
    WavetableSynthDialog {
        id: wavetableSynthDialog
        anchors.centerIn: parent
        width: parent.width * Constants.largeDialogScale
        height: parent.height * Constants.largeDialogScale
    }
    BassSynthDialog {
        id: bassSynthDialog
        anchors.centerIn: parent
        width: parent.width * Constants.largeDialogScale
        height: parent.height * Constants.largeDialogScale
    }
    DrumSynthDialog {
        id: drumSynthDialog
        anchors.centerIn: parent
        width: parent.width * Constants.largeDialogScale
        height: parent.height * Constants.largeDialogScale
    }
    DeviceRackDialog {
        id: deviceRackDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
        onClosed: {
            if (_deviceNamesBeforeRackOpen === null) {
                return;
            }
            const before = _deviceNamesBeforeRackOpen;
            _deviceNamesBeforeRackOpen = null;
            for (let i = 0; i < deviceRackController.deviceCount; i++) {
                const name = deviceRackController.deviceName(i);
                if (name !== "" && before.indexOf(name) === -1 && name !== trackSettingsModel.portName) {
                    _newDeviceName = name;
                    portAutoAssignDialog.message = qsTr("'%1' was added. Assign it as the instrument port for this track?").arg(name);
                    portAutoAssignDialog.open();
                    break;
                }
            }
        }
    }

    MasterEffectsDialog {
        id: masterEffectsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }

    DeviceInsertEffectsDialog {
        id: deviceInsertEffectsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }

    EffectSendsDialog {
        id: effectSendsDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
    }
    ReverbDialog {
        id: reverbDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    ChorusDialog {
        id: chorusDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    PannerDialog {
        id: pannerDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    AutoPannerDialog {
        id: autoPannerDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    AllPassFilterDialog {
        id: allPassFilterDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    LufsMeterDialog {
        id: lufsMeterDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    DbtpMeterDialog {
        id: dbtpMeterDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    CompressorDialog {
        id: compressorDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    DelayDialog {
        id: delayDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    ClipperDialog {
        id: clipperDialog
        anchors.centerIn: parent
        width: parent.width * Constants.effectDialogScale
        height: parent.height * Constants.effectDialogScale
    }
    Eq8BandParametricDialog {
        id: eq8BandParametricDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
    ManualDialog {
        id: manualDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        height: parent.height * Constants.defaultDialogScale
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
        onAccepted: {
            UiService.interpolationStartValue = startValue();
            UiService.interpolationEndValue = endValue();
            UiService.interpolationUsePercentages = usePercentages();
            editorService.requestLinearVelocityInterpolationOnColumn(startLine(), endLine(), startValue(), endValue(), usePercentages());
        }
    }
    InterpolationDialog {
        id: trackVelocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: {
            UiService.interpolationStartLine = startLine();
            UiService.interpolationEndLine = endLine();
            UiService.interpolationStartValue = startValue();
            UiService.interpolationEndValue = endValue();
            UiService.interpolationUsePercentages = usePercentages();
            editorService.requestLinearVelocityInterpolationOnTrack(startLine(), endLine(), startValue(), endValue(), usePercentages());
        }
    }
    InterpolationDialog {
        id: selectionVelocityInterpolationDialog
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: {
            UiService.interpolationStartValue = startValue();
            UiService.interpolationEndValue = endValue();
            UiService.interpolationUsePercentages = usePercentages();
            editorService.requestLinearVelocityInterpolationOnSelection(startLine(), endLine(), startValue(), endValue(), usePercentages());
        }
    }
    AddMidiCcAutomationDialog {
        id: addMidiCcAutomationDialog
        portName: editorService.instrumentPortName(editorService.position.track)
        anchors.centerIn: parent
        width: parent.width * Constants.defaultDialogScale
        onAccepted: {
            const position = editorService.position;
            const automationId = automationService.addMidiCcAutomation(position.pattern, position.track, position.column, addMidiCcAutomationDialog.controller(), addMidiCcAutomationDialog.startLine(), addMidiCcAutomationDialog.endLine(), addMidiCcAutomationDialog.startValue(), addMidiCcAutomationDialog.endValue(), addMidiCcAutomationDialog.comment(), true, addMidiCcAutomationDialog.eventsPerBeat(), addMidiCcAutomationDialog.lineOffset());
            if (addMidiCcAutomationDialog.cycles() > 0 || addMidiCcAutomationDialog.amplitude() > 0 || addMidiCcAutomationDialog.offset() !== 0) {
                automationService.addMidiCcModulation(automationId, addMidiCcAutomationDialog.modulationType(), addMidiCcAutomationDialog.cycles(), addMidiCcAutomationDialog.amplitude(), addMidiCcAutomationDialog.offset(), addMidiCcAutomationDialog.inverted());
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
            const automationId = automationService.addPitchBendAutomation(position.pattern, position.track, position.column, startLine(), endLine(), startValue(), endValue(), comment());
            if (addPitchBendAutomationDialog.cycles() > 0 || addPitchBendAutomationDialog.amplitude() > 0 || addPitchBendAutomationDialog.offset() !== 0) {
                automationService.addPitchBendModulation(automationId, addPitchBendAutomationDialog.modulationType(), addPitchBendAutomationDialog.cycles(), addPitchBendAutomationDialog.amplitude(), addPitchBendAutomationDialog.offset(), addPitchBendAutomationDialog.inverted());
            }
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
    DeleteUnusedPatternsDialog {
        id: deleteUnusedPatternsDialog
        anchors.centerIn: parent
    }
    MainContextMenu {
        id: contextMenu
        width: parent.width * 0.25
    }
    function _getWindowTitle(): string {
        const nameAndVersion = `${applicationService.applicationName()} MIDI tracker v${applicationService.applicationVersion()}`;
        const currentFileName = (editorService.currentFileName ? " - " + editorService.currentFileName : "");
        const modifiedIndicator = (editorService.isModified ? " (*)" : "");
        return `${nameAndVersion}${currentFileName}${modifiedIndicator}`;
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
        applicationService.audioRenderDialogRequested.connect(audioRenderDialog.open);
        applicationService.masterEffectsDialogRequested.connect(masterEffectsDialog.open);
        applicationService.midiExportDialogRequested.connect(midiExportDialog.open);
        applicationService.midiImportDialogRequested.connect(midiImportDialog.open);
        applicationService.openDialogRequested.connect(openDialog.open);
        applicationService.recentFilesDialogRequested.connect(recentFilesDialog.open);
        applicationService.saveAsDialogRequested.connect(saveAsDialog.open);
        applicationService.saveAsTemplateDialogRequested.connect(saveAsTemplateDialog.open);
        applicationService.statusTextRequested.connect(bottomBar.setStatusText);
        applicationService.unsavedChangesDialogRequested.connect(unsavedChangesDialog.open);
        deviceRackController.bassSynthDialogRequested.connect(bassSynthDialog.open);
        deviceRackController.drumSynthDialogRequested.connect(drumSynthDialog.open);
        deviceRackController.samplerDialogRequested.connect(samplerDialog.open);
        deviceRackController.synthDialogRequested.connect(synthDialog.open);
        deviceRackController.wavetableSynthDialogRequested.connect(wavetableSynthDialog.open);
        deviceRackController.importSettingsConfirmationRequested.connect(UiService.requestImportDeviceSettingsConfirmation);
        effectRackController.importEffectSettingsConfirmationRequested.connect(UiService.requestImportEffectSettingsConfirmation);
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
        UiService.manualDialogRequested.connect(manualDialog.open);
        UiService.shortcutsDialogRequested.connect(shortcutsDialog.open);
        UiService.eventSelectionDialogRequested.connect(() => {
            eventSelectionDialog.requestData();
            eventSelectionDialog.open();
        });
        UiService.focusOnEditorViewRequested.connect(() => {
            uiLogger.info(_tag, "Settings focus on editor view");
            _editorView.focus = true;
        });
        UiService.deviceRackDialogRequested.connect(() => {
            deviceRackDialog.updateUsage();
            deviceRackDialog.open();
        });
        UiService.deviceRackDialogFromTrackSettingsRequested.connect(() => {
            const names = [];
            for (let i = 0; i < deviceRackController.deviceCount; i++) {
                const name = deviceRackController.deviceName(i);
                if (name !== "")
                    names.push(name);
            }
            _deviceNamesBeforeRackOpen = names;
            deviceRackDialog.updateUsage();
            deviceRackDialog.open();
        });
        UiService.effectSendsDialogRequested.connect(deviceName => {
            effectSendsDialog.deviceName = deviceName;
            effectSendsDialog.open();
        });
        UiService.deviceInsertEffectsDialogRequested.connect(deviceName => {
            deviceInsertEffectsDialog.deviceName = deviceName;
            deviceInsertEffectsDialog.open();
        });

        UiService.exportDeviceSettingsRequested.connect((slotIndex, deviceName, deviceTypeName) => {
            exportDeviceSettingsDialog.slotIndex = slotIndex;
            const filename = applicationService.defaultDeviceFileName(deviceTypeName) + applicationService.deviceSettingsExtension();
            exportDeviceSettingsDialog.currentFile = exportDeviceSettingsDialog.currentFolder + "/" + filename;
            exportDeviceSettingsDialog.open();
        });
        UiService.importDeviceSettingsRequested.connect(slotIndex => {
            importDeviceSettingsDialog.slotIndex = slotIndex;
            importDeviceSettingsDialog.open();
        });
        UiService.exportEffectSettingsRequested.connect((slotIndex, effectType) => {
            exportEffectSettingsDialog.slotIndex = slotIndex;
            const filename = applicationService.defaultEffectFileName(effectType) + applicationService.effectRackSettingsExtension();
            exportEffectSettingsDialog.currentFile = exportEffectSettingsDialog.currentFolder + "/" + filename;
            exportEffectSettingsDialog.open();
        });
        UiService.importEffectSettingsRequested.connect(slotIndex => {
            importEffectSettingsDialog.slotIndex = slotIndex;
            importEffectSettingsDialog.open();
        });
        UiService.importEffectSettingsConfirmationRequested.connect((slotIndex, fileUrl, currentType, importedType, typeMismatch) => {
            if (typeMismatch) {
                importEffectSettingsConfirmationDialog.slotIndex = slotIndex;
                importEffectSettingsConfirmationDialog.fileUrl = fileUrl;
                importEffectSettingsConfirmationDialog.message = qsTr("The file contains settings for '%1' but the current effect is '%2'. Replace it anyway?").arg(importedType).arg(currentType);
                importEffectSettingsConfirmationDialog.open();
            } else {
                effectRackController.confirmImportEffectSettings(slotIndex, fileUrl);
            }
        });
        UiService.importDeviceSettingsConfirmationRequested.connect((slotIndex, fileUrl, currentTypeName, importedTypeName, typeMismatch) => {
            if (typeMismatch) {
                importDeviceSettingsConfirmationDialog.slotIndex = slotIndex;
                importDeviceSettingsConfirmationDialog.fileUrl = fileUrl;
                importDeviceSettingsConfirmationDialog.message = qsTr("The file contains settings for '%1' but the current device is '%2'. Replace it anyway?").arg(importedTypeName).arg(currentTypeName);
                importDeviceSettingsConfirmationDialog.open();
            } else {
                deviceRackController.confirmImportSettings(slotIndex, fileUrl);
            }
        });

        UiService.samplerDialogRequested.connect(samplerDialog.open);
        UiService.drumSynthDialogRequested.connect(drumSynthDialog.open);
        UiService.recentFilesDialogRequested.connect(recentFilesDialog.open);
        UiService.deviceDialogRequested.connect(deviceName => {
            if (deviceService.isInternalDevice(deviceName)) {
                deviceRackController.openDevice(deviceName);
            } else if (deviceName === applicationService.samplerDeviceName) {
                samplerDialog.open();
            } else if (deviceName === applicationService.synthDeviceName) {
                synthDialog.open();
            } else if (deviceName === applicationService.drumSynthDeviceName) {
                drumSynthDialog.open();
            }
        });
        UiService.settingsDialogRequested.connect(settingsDialog.open);
        UiService.columnSettingsDialogRequested.connect((trackIndex, columnIndex) => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
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
            columnVelocityInterpolationDialog.setStartValue(UiService.interpolationStartValue);
            columnVelocityInterpolationDialog.setEndValue(UiService.interpolationEndValue);
            columnVelocityInterpolationDialog.setUsePercentages(UiService.interpolationUsePercentages);
            columnVelocityInterpolationDialog.open();
        });
        UiService.selectionVelocityInterpolationDialogRequested.connect(() => {
            selectionVelocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
            selectionVelocityInterpolationDialog.setStartLine(selectionService.minLine());
            selectionVelocityInterpolationDialog.setEndLine(selectionService.maxLine());
            selectionVelocityInterpolationDialog.setStartValue(UiService.interpolationStartValue);
            selectionVelocityInterpolationDialog.setEndValue(UiService.interpolationEndValue);
            selectionVelocityInterpolationDialog.setUsePercentages(UiService.interpolationUsePercentages);
            selectionVelocityInterpolationDialog.open();
        });
        UiService.trackVelocityInterpolationDialogRequested.connect(() => {
            trackVelocityInterpolationDialog.setTitle(qsTr("Interpolate velocity"));
            trackVelocityInterpolationDialog.setStartLine(UiService.interpolationStartLine);
            trackVelocityInterpolationDialog.setEndLine(UiService.interpolationEndLine === 0 ? editorService.currentLineCount - 1 : UiService.interpolationEndLine);
            trackVelocityInterpolationDialog.setStartValue(UiService.interpolationStartValue);
            trackVelocityInterpolationDialog.setEndValue(UiService.interpolationEndValue);
            trackVelocityInterpolationDialog.setUsePercentages(UiService.interpolationUsePercentages);
            trackVelocityInterpolationDialog.open();
        });
        UiService.lineDelayDialogRequested.connect(() => {
            lineDelayDialog.setValue(editorService.delayAtCurrentPosition());
            lineDelayDialog.open();
        });
        UiService.lineAddMidiCcAutomationDialogRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setPortName(editorService.instrumentPortName(editorService.position.track));
            addMidiCcAutomationDialog.setStartLine(editorService.position.line);
            addMidiCcAutomationDialog.setEndLine(editorService.position.line);
            addMidiCcAutomationDialog.setStartValue(100);
            addMidiCcAutomationDialog.setEndValue(0);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.resetOutput();
            addMidiCcAutomationDialog.open();
        });
        UiService.columnAddMidiCcAutomationDialogRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setPortName(editorService.instrumentPortName(editorService.position.track));
            addMidiCcAutomationDialog.setStartLine(0);
            addMidiCcAutomationDialog.setEndLine(editorService.currentLineCount - 1);
            addMidiCcAutomationDialog.setStartValue(0);
            addMidiCcAutomationDialog.setEndValue(100);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.resetOutput();
            addMidiCcAutomationDialog.open();
        });
        UiService.selectionAddMidiCcAutomationDialogRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            addMidiCcAutomationDialog.setTitle(qsTr("Add MIDI CC automation"));
            addMidiCcAutomationDialog.setPortName(editorService.instrumentPortName(editorService.position.track));
            addMidiCcAutomationDialog.setStartLine(selectionService.minLine());
            addMidiCcAutomationDialog.setEndLine(selectionService.maxLine());
            addMidiCcAutomationDialog.setStartValue(0);
            addMidiCcAutomationDialog.setEndValue(100);
            addMidiCcAutomationDialog.setComment("");
            addMidiCcAutomationDialog.resetModulations();
            addMidiCcAutomationDialog.resetOutput();
            addMidiCcAutomationDialog.open();
        });
        UiService.editMidiCcAutomationsDialogRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            midiCcAutomationsModel.requestMidiCcAutomations();
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations"));
            editMidiCcAutomationsDialog.open();
        });
        UiService.editMidiCcAutomationsDialogByLineRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            const position = editorService.position;
            midiCcAutomationsModel.requestMidiCcAutomationsByLine(position.pattern, position.track, position.column, position.line);
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by line"));
            editMidiCcAutomationsDialog.open();
        });
        UiService.editMidiCcAutomationsDialogByColumnRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            const position = editorService.position;
            midiCcAutomationsModel.requestMidiCcAutomationsByColumn(position.pattern, position.track, position.column);
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by column"));
            editMidiCcAutomationsDialog.open();
        });
        UiService.editMidiCcAutomationsDialogByTrackRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
            const position = editorService.position;
            midiCcAutomationsModel.requestMidiCcAutomationsByTrack(position.pattern, position.track);
            editMidiCcAutomationsDialog.setTitle(qsTr("Edit MIDI CC automations by track"));
            editMidiCcAutomationsDialog.open();
        });
        UiService.editMidiCcAutomationsDialogByPatternRequested.connect(() => {
            midiCcAutomationsModel.linesPerBeat = editorService.linesPerBeat;
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
        UiService.effectsGalleryDialogRequested.connect(slotIndex => {
            effectsGalleryDialog.slotIndex = slotIndex;
            effectsGalleryDialog.open();
        });
        UiService.deviceGalleryDialogRequested.connect(slotIndex => {
            deviceGalleryDialog.slotIndex = slotIndex;
            deviceGalleryDialog.open();
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
        UiService.deleteUnusedPatternsRequested.connect(() => {
            deleteUnusedPatternsDialog.open();
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
        if (_editorView) {
            _editorView.resize(editorViewContainer.width, editorViewContainer.height);
        }
        if (_songView) {
            _songView.width = songViewContainer.width;
            _songView.height = songViewContainer.height;
        }
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
