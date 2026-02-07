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
    signal eventSelectionDialogRequested
    signal focusOnEditorViewRequested
    signal gainConverterDialogRequested
    signal lineAddMidiCcAutomationDialogRequested
    signal lineAddPitchBendAutomationDialogRequested
    signal lineDelayDialogRequested
    signal noteFrequencyDialogRequested
    signal recentFilesDialogRequested
    signal selectionAddMidiCcAutomationDialogRequested
    signal selectionAddPitchBendAutomationDialogRequested
    signal selectionVelocityInterpolationDialogRequested
    signal settingsDialogRequested
    signal shortcutsDialogRequested
    signal trackSettingsDialogRequested(int trackIndex)
    signal trackVelocityInterpolationDialogRequested
    signal trackVelocityScaleDialogRequested(int trackIndex)
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
    function requestShortcutsDialog(): void {
        shortcutsDialogRequested();
    }
    function requestDelayCalculatorDialog(): void {
        delayCalculatorDialogRequested();
    }
    function requestEventSelectionDialog(): void {
        eventSelectionDialogRequested();
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
    signal quitRequested
    function requestQuit(): void {
        quitRequested();
    }
}
