pragma Singleton
import QtQuick 2.15

QtObject {
    signal aboutDialogRequested
    signal activeOctaveChanged(int activeOctave)
    signal columnVelocityScaleDialogRequested(int trackIndex, int columnIndex)
    signal eventSelectionDialogRequested
    signal focusOnEditorViewRequested
    signal recentFilesDialogRequested
    signal columnVelocityInterpolationDialogRequested
    signal selectionVelocityInterpolationDialogRequested
    signal settingsDialogRequested
    signal trackSettingsDialogRequested(int trackIndex)
    signal trackVelocityScaleDialogRequested(int trackIndex)
    property int _activeOctave: 3
    readonly property string _tag: "UiService"
    function activeOctave() {
        return _activeOctave;
    }
    function setActiveOctave(octave) {
        if (0 >= octave <= 8 && _activeOctave !== octave) {
            _activeOctave = octave;
            activeOctaveChanged(_activeOctave);
        }
    }
    signal activeStepChanged(int activeStep)
    property int _activeStep: config.step(1)
    function activeStep() {
        return _activeStep;
    }
    function setActiveStep(step) {
        if (_activeStep !== step) {
            _activeStep = step;
            config.setStep(step);
            activeStepChanged(_activeStep);
        }
    }
    signal activeVelocityChanged(int activeVelocity)
    property int _activeVelocity: config.velocity(100)
    function activeVelocity() {
        return _activeVelocity;
    }
    function setActiveVelocity(velocity) {
        if (_activeVelocity !== velocity) {
            _activeVelocity = velocity;
            config.setVelocity(velocity);
            activeVelocityChanged(_activeVelocity);
        }
    }
    signal editModeChanged(int editMode)
    property bool _editMode: false
    function editMode() {
        return _editMode;
    }
    function setEditMode(editMode) {
        if (_editMode !== editMode && (!editMode || !isPlaying())) {
            _editMode = editMode;
            editModeChanged(_editMode);
        }
    }
    function toggleEditMode() {
        setEditMode(!editMode());
    }
    function isPlaying() {
        return playerService.isPlaying;
    }
    function togglePlay() {
        if (!isPlaying()) {
            if (requestPlay()) {
                setEditMode(false);
            }
        } else {
            requestStop();
        }
    }
    function requestPlay() {
        uiLogger.debug(_tag, "Requesting play");
        return playerService.play();
    }
    function requestStop() {
        uiLogger.debug(_tag, "Requesting stop");
        playerService.stop();
    }
    function setIsLooping(isLooping) {
        uiLogger.debug(_tag, `Setting pattern looping: ${isLooping}`);
        playerService.setIsLooping(isLooping);
    }
    function requestAboutDialog() {
        aboutDialogRequested();
    }
    function requestEventSelectionDialog() {
        eventSelectionDialogRequested();
    }
    function requestRecentFilesDialog() {
        recentFilesDialogRequested();
    }
    function requestSettingsDialog() {
        settingsDialogRequested();
    }
    function requestTrackSettingsDialog(trackIndex) {
        trackSettingsDialogRequested(trackIndex);
    }
    function requestColumnVelocityScaleDialog(trackIndex, columnIndex) {
        if (!isPlaying()) {
            columnVelocityScaleDialogRequested(trackIndex, columnIndex);
        }
    }
    function requestTrackVelocityScaleDialog(trackIndex) {
        if (!isPlaying()) {
            trackVelocityScaleDialogRequested(trackIndex);
        }
    }
    function requestColumnVelocityInterpolationDialog() {
        if (!isPlaying()) {
            columnVelocityInterpolationDialogRequested();
        }
    }
    function requestSelectionVelocityInterpolationDialog() {
        if (!isPlaying()) {
            selectionVelocityInterpolationDialogRequested();
        }
    }
    function requestFocusOnEditorView() {
        focusOnEditorViewRequested();
    }
    function rewindSong() {
        editorService.resetSongPosition();
        applicationService.applyAllTrackSettings();
    }
    signal quitRequested
    function requestQuit() {
        quitRequested();
    }
}
