pragma Singleton
import QtQuick 2.15

QtObject {
    signal aboutDialogRequested
    signal activeOctaveChanged(int activeOctave)
    signal focusOnEditorViewRequested
    signal trackSettingsDialogRequested(int trackIndex)
    property int _activeOctave: 3
    readonly property string _tag: "UiService"
    function activeOctave() {
        return _activeOctave;
    }
    function setActiveOctave(octave) {
        if (_activeOctave !== octave) {
            _activeOctave = octave;
            activeOctaveChanged(_activeOctave);
        }
    }
    signal activeStepChanged(int activeStep)
    property int _activeStep: config.loadStep(1)
    function activeStep() {
        return _activeStep;
    }
    function setActiveStep(step) {
        if (_activeStep !== step) {
            _activeStep = step;
            config.saveStep(step);
            activeStepChanged(_activeStep);
        }
    }
    signal activeVelocityChanged(int activeVelocity)
    property int _activeVelocity: config.loadVelocity(100)
    function activeVelocity() {
        return _activeVelocity;
    }
    function setActiveVelocity(velocity) {
        if (_activeVelocity !== velocity) {
            _activeVelocity = velocity;
            config.saveVelocity(velocity);
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
        return playerService.requestPlay();
    }
    function requestStop() {
        uiLogger.debug(_tag, "Requesting stop");
        playerService.requestStop();
    }
    function requestAboutDialog() {
        aboutDialogRequested();
    }
    function requestTrackSettingsDialog(trackIndex) {
        trackSettingsDialogRequested(trackIndex);
    }
    function requestFocusOnEditorView() {
        focusOnEditorViewRequested();
    }
    signal quitRequested
    function requestQuit() {
        quitRequested();
    }
}
