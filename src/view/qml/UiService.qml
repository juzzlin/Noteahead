pragma Singleton
import QtQuick 2.15

QtObject {
    signal aboutDialogRequested
    signal activeOctaveChanged(int activeOctave)
    signal columnAddMidiCcAutomationDialogRequested
    signal columnAddPitchBendAutomationDialogRequested
    signal columnSettingsDialogRequested(int trackIndex, int columnIndex)
    signal columnVelocityInterpolationDialogRequested
    signal columnVelocityScaleDialogRequested(int trackIndex, int columnIndex)
    signal contextMenuRequested(int globalX, int globalY)
    signal delayCalculatorDialogRequested
    signal deleteUnusedPatternsConfirmed
    signal deleteUnusedPatternsRequested
    signal deviceDialogRequested(string deviceName)
    signal deviceGalleryDialogRequested(int slotIndex)
    signal deviceInsertEffectsDialogRequested(string deviceName)
    signal deviceRackDialogFromTrackSettingsRequested
    signal deviceRackDialogRequested
    signal exportDeviceSettingsRequested(int slotIndex, string deviceName, string deviceTypeName)
    signal importDeviceSettingsRequested(int slotIndex)
    signal exportEffectSettingsRequested(int slotIndex, string effectType)
    signal importEffectSettingsRequested(int slotIndex)
    signal importEffectSettingsConfirmationRequested(int slotIndex, url fileUrl, string currentType, string importedType, bool typeMismatch)
    signal importDeviceSettingsConfirmationRequested(int slotIndex, url fileUrl, string currentTypeName, string importedTypeName, bool typeMismatch)
    signal drumSynthDialogRequested
    signal editMidiCcAutomationsDialogByColumnRequested
    signal editMidiCcAutomationsDialogByLineRequested
    signal editMidiCcAutomationsDialogByPatternRequested
    signal editMidiCcAutomationsDialogByTrackRequested
    signal editMidiCcAutomationsDialogRequested
    signal editPitchBendAutomationsDialogByColumnRequested
    signal editPitchBendAutomationsDialogByLineRequested
    signal editPitchBendAutomationsDialogByPatternRequested
    signal editPitchBendAutomationsDialogByTrackRequested
    signal editPitchBendAutomationsDialogRequested
    signal effectSendsDialogRequested(string deviceName)
    signal effectsGalleryDialogRequested(int slotIndex)
    signal eventSelectionDialogRequested
    signal focusOnEditorViewRequested
    signal gainConverterDialogRequested
    signal lineAddMidiCcAutomationDialogRequested
    signal lineAddPitchBendAutomationDialogRequested
    signal lineDelayDialogRequested
    signal manualDialogRequested
    signal noteFrequencyDialogRequested
    signal recentFilesDialogRequested
    signal samplerDialogRequested
    signal selectionAddMidiCcAutomationDialogRequested
    signal selectionAddPitchBendAutomationDialogRequested
    signal selectionVelocityInterpolationDialogRequested
    signal settingsDialogRequested
    signal shortcutsDialogRequested
    signal trackSettingsDialogRequested(int trackIndex)
    signal trackVelocityInterpolationDialogRequested
    signal trackVelocityScaleDialogRequested(int trackIndex)
    signal columnPanInterpolationDialogRequested
    property int _activeOctave: keyboardService.activeOctave
    on_ActiveOctaveChanged: activeOctaveChanged(_activeOctave)
    readonly property string _tag: "UiService"
    function activeOctave(): int {
        return _activeOctave;
    }
    function setActiveOctave(octave: int): void {
        keyboardService.activeOctave = octave;
    }
    signal activeStepChanged(int activeStep)
    property int _activeStep: settingsService.step(1)
    function activeStep(): int {
        return _activeStep;
    }
    function setActiveStep(step): void {
        if (_activeStep !== step) {
            _activeStep = step;
            settingsService.setStep(step);
            activeStepChanged(_activeStep);
        }
    }
    signal activeVelocityChanged(int activeVelocity)
    property int _activeVelocity: settingsService.velocity(100)

    Component.onCompleted: {
        applicationService.deviceRackDialogRequested.connect(deviceRackDialogRequested);
        applicationService.samplerDialogRequested.connect(samplerDialogRequested);
        applicationService.drumSynthDialogRequested.connect(drumSynthDialogRequested);
    }

    function activeVelocity(): int {
        return _activeVelocity;
    }
    function setActiveVelocity(velocity): void {
        if (_activeVelocity !== velocity) {
            _activeVelocity = velocity;
            settingsService.setVelocity(velocity);
            activeVelocityChanged(_activeVelocity);
        }
    }
    function isPlaying(): bool {
        return playerService.isPlaying;
    }
    function togglePlay(): void {
        if (!isPlaying()) {
            requestPlay();
        } else {
            requestStop();
        }
    }
    function requestPlay(): bool {
        uiLogger.debug(_tag, "Requesting play");
        return playerService.play();
    }
    function requestStop(): void {
        uiLogger.debug(_tag, "Requesting stop");
        playerService.stop();
    }
    function setIsLooping(isLooping): void {
        uiLogger.debug(_tag, `Setting pattern looping: ${isLooping}`);
        playerService.setIsLooping(isLooping);
    }
    function jumpToSongPosition(index: int): void {
        if (isPlaying()) {
            requestStop();
            editorService.setSongPosition(index);
            requestPlay();
        } else {
            editorService.setSongPosition(index);
        }
    }
    function requestAboutDialog(): void {
        aboutDialogRequested();
    }
    function requestManualDialog(): void {
        manualDialogRequested();
    }
    function requestShortcutsDialog(): void {
        shortcutsDialogRequested();
    }
    function requestDelayCalculatorDialog(): void {
        delayCalculatorDialogRequested();
    }
    function requestEventSelectionDialog(): void {
        eventSelectionDialogRequested();
    }
    function requestEffectsGalleryDialog(slotIndex: int): void {
        effectsGalleryDialogRequested(slotIndex);
    }
    function requestDeviceGalleryDialog(slotIndex: int): void {
        deviceGalleryDialogRequested(slotIndex);
    }
    function requestGainConverterDialog(): void {
        gainConverterDialogRequested();
    }
    function requestNoteFrequencyDialog(): void {
        noteFrequencyDialogRequested();
    }
    function requestRecentFilesDialog(): void {
        recentFilesDialogRequested();
    }
    function requestDeviceRackDialog(): void {
        deviceRackDialogRequested();
    }
    function requestDeviceRackDialogFromTrackSettings(): void {
        deviceRackDialogFromTrackSettingsRequested();
    }
    function requestExportDeviceSettings(slotIndex: int, deviceName: string, deviceTypeName: string): void {
        exportDeviceSettingsRequested(slotIndex, deviceName, deviceTypeName);
    }
    function requestImportDeviceSettings(slotIndex: int): void {
        importDeviceSettingsRequested(slotIndex);
    }
    function requestExportEffectSettings(slotIndex: int, effectType: string): void {
        exportEffectSettingsRequested(slotIndex, effectType);
    }
    function requestImportEffectSettings(slotIndex: int): void {
        importEffectSettingsRequested(slotIndex);
    }
    function requestImportEffectSettingsConfirmation(slotIndex: int, fileUrl: url, currentType: string, importedType: string, typeMismatch: bool): void {
        importEffectSettingsConfirmationRequested(slotIndex, fileUrl, currentType, importedType, typeMismatch);
    }
    function requestImportDeviceSettingsConfirmation(slotIndex: int, fileUrl: url, currentTypeName: string, importedTypeName: string, typeMismatch: bool): void {
        importDeviceSettingsConfirmationRequested(slotIndex, fileUrl, currentTypeName, importedTypeName, typeMismatch);
    }
    function requestEffectSendsDialog(deviceName: string): void {
        effectSendsDialogRequested(deviceName);
    }
    function requestDeviceInsertEffectsDialog(deviceName: string): void {
        deviceInsertEffectsDialogRequested(deviceName);
    }
    function requestDrumSynthDialog(): void {
        drumSynthDialogRequested();
    }
    function requestDeviceDialog(deviceName: string): void {
        deviceDialogRequested(deviceName);
    }
    function requestSettingsDialog(): void {
        settingsDialogRequested();
    }
    function requestColumnSettingsDialog(trackIndex, columnIndex): void {
        columnSettingsDialogRequested(trackIndex, columnIndex);
    }
    function requestTrackSettingsDialog(trackIndex): void {
        trackSettingsDialogRequested(trackIndex);
    }
    function requestColumnVelocityScaleDialog(trackIndex, columnIndex): void {
        if (!isPlaying()) {
            columnVelocityScaleDialogRequested(trackIndex, columnIndex);
        }
    }
    function requestTrackVelocityScaleDialog(trackIndex): void {
        if (!isPlaying()) {
            trackVelocityScaleDialogRequested(trackIndex);
        }
    }
    function requestColumnVelocityInterpolationDialog(): void {
        if (!isPlaying()) {
            columnVelocityInterpolationDialogRequested();
        }
    }
    function requestTrackVelocityInterpolationDialog(): void {
        if (!isPlaying()) {
            trackVelocityInterpolationDialogRequested();
        }
    }
    function requestColumnPanInterpolationDialog(): void {
        if (!isPlaying()) {
            columnPanInterpolationDialogRequested();
        }
    }
    function requestSelectionVelocityInterpolationDialog(): void {
        if (!isPlaying()) {
            selectionVelocityInterpolationDialogRequested();
        }
    }
    function requestLineDelayDialog(): void {
        lineDelayDialogRequested();
    }
    function requestLineAddMidiCcAutomationDialog(): void {
        if (!isPlaying()) {
            lineAddMidiCcAutomationDialogRequested();
        }
    }
    function requestColumnAddMidiCcAutomationDialog(): void {
        if (!isPlaying()) {
            columnAddMidiCcAutomationDialogRequested();
        }
    }
    function requestSelectionAddMidiCcAutomationDialog(): void {
        if (!isPlaying()) {
            selectionAddMidiCcAutomationDialogRequested();
        }
    }
    function requestEditMidiCcAutomationsDialog(): void {
        if (!isPlaying()) {
            editMidiCcAutomationsDialogRequested();
        }
    }
    function requestEditMidiCcAutomationsDialogByLine(): void {
        if (!isPlaying()) {
            editMidiCcAutomationsDialogByLineRequested();
        }
    }
    function requestEditMidiCcAutomationsDialogByColumn(): void {
        if (!isPlaying()) {
            editMidiCcAutomationsDialogByColumnRequested();
        }
    }
    function requestEditMidiCcAutomationsDialogByTrack(): void {
        if (!isPlaying()) {
            editMidiCcAutomationsDialogByTrackRequested();
        }
    }
    function requestEditMidiCcAutomationsDialogByPattern(): void {
        if (!isPlaying()) {
            editMidiCcAutomationsDialogByPatternRequested();
        }
    }
    function requestLineAddPitchBendAutomationDialog(): void {
        if (!isPlaying()) {
            lineAddPitchBendAutomationDialogRequested();
        }
    }
    function requestColumnAddPitchBendAutomationDialog(): void {
        if (!isPlaying()) {
            columnAddPitchBendAutomationDialogRequested();
        }
    }
    function requestSelectionAddPitchBendAutomationDialog(): void {
        if (!isPlaying()) {
            selectionAddPitchBendAutomationDialogRequested();
        }
    }
    function requestEditPitchBendAutomationsDialog(): void {
        if (!isPlaying()) {
            editPitchBendAutomationsDialogRequested();
        }
    }
    function requestEditPitchBendAutomationsDialogByLine(): void {
        if (!isPlaying()) {
            editPitchBendAutomationsDialogByLineRequested();
        }
    }
    function requestEditPitchBendAutomationsDialogByColumn(): void {
        if (!isPlaying()) {
            editPitchBendAutomationsDialogByColumnRequested();
        }
    }
    function requestEditPitchBendAutomationsDialogByTrack(): void {
        if (!isPlaying()) {
            editPitchBendAutomationsDialogByTrackRequested();
        }
    }
    function requestEditPitchBendAutomationsDialogByPattern(): void {
        if (!isPlaying()) {
            editPitchBendAutomationsDialogByPatternRequested();
        }
    }
    function requestFocusOnEditorView(): void {
        focusOnEditorViewRequested();
    }
    function requestContextMenu(globalX: int, globalY: int): void {
        contextMenuRequested(globalX, globalY);
    }
    function rewindSong(): void {
        editorService.resetSongPosition();
        applicationService.applyAllTrackSettings();
    }
    function requestDeleteUnusedPatterns(): void {
        if (!isPlaying()) {
            deleteUnusedPatternsRequested();
        }
    }
    function confirmDeleteUnusedPatterns(): void {
        uiLogger.info("UiService", "Confirming unused pattern deletion");
        deleteUnusedPatternsConfirmed();
    }
    property int interpolationStartLine: 0
    property int interpolationEndLine: 0
    property int interpolationStartValue: 0
    property int interpolationEndValue: 100
    property bool interpolationUsePercentages: false

    signal quitRequested
    function requestQuit(): void {
        quitRequested();
    }
}
