import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

FocusScope {
    id: rootItem
    property var _patterns: []
    readonly property string _tag: "EditorView"
    focus: true
    Component {
        id: patternComponent
        Pattern {
            height: rootItem.height
        }
    }
    LineNumberColumn {
        id: lineNumberColumnLeft
        height: _lineNumberColumnHeight()
        width: _lineNumberColumnWidth()
        anchors.top: parent.top
        anchors.topMargin: Constants.trackHeaderHeight + Constants.columnHeaderHeight
        anchors.left: parent.left
    }
    Rectangle {
        id: trackArea
        height: rootItem.height
        anchors.top: parent.top
        anchors.left: lineNumberColumnLeft.right
        anchors.right: lineNumberColumnRight.left
        clip: true
        color: "black"
    }
    PositionBar {
        id: positionBar
        width: rootItem.width
        height: _positionBarHeight()
        x: 0
        y: _positionBarY()
        z: 10
        border.width: applicationService.editMode ? 2 : 0
        border.color: Constants.positionBarBorderColorEditMode
    }
    LineNumberColumn {
        id: lineNumberColumnRight
        height: _lineNumberColumnHeight()
        width: _lineNumberColumnWidth()
        anchors.top: parent.top
        anchors.topMargin: Constants.trackHeaderHeight + Constants.columnHeaderHeight
        anchors.right: parent.right
    }
    KeyboardHandler {
        id: keyboardHandler
    }
    MouseHandler {
        id: mouseHandler
    }
    ScrollBar {
        id: horizontalScrollBar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Horizontal
        size: editorService.scrollBarHandleSize
        stepSize: editorService.scrollBarStepSize
        snapMode: ScrollBar.SnapAlways
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        onPositionChanged: {
            editorService.requestHorizontalScrollBarPositionChange(position);
        }
        Component.onCompleted: {
            // Keyboard commands like tab/left/right might force also scroll bar position
            editorService.scrollBarPositionChanged.connect(() => {
                position = editorService.scrollBarPosition();
            });
        }
    }
    Keys.onPressed: event => {
        keyboardHandler.handleKeyPressed(event);
        event.accepted = true;
    }
    Keys.onReleased: event => {
        keyboardHandler.handleKeyReleased(event);
        event.accepted = true;
    }
    function initialize() {
        _connectSignals();
        _recreatePatterns();
        _createLineColumns();
        editorService.requestPosition(0, 0, 0, 0, 0);
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _updateCurrentTrackDimensions();
        _updateLineColumns();
        resizeTimer.restart();
    }
    readonly property int _resizeDelay: 500 // milliseconds
    Timer {
        id: resizeTimer
        interval: _resizeDelay
        repeat: false
        onTriggered: _updateAllTrackDimensions()
    }
    function _clearPatterns() {
        _patterns.forEach(pattern => {
            pattern.destroy();
        });
        _patterns.length = 0;
    }
    function _setTrackDimensions(track) {
        const unitWidth = trackArea.width / editorService.visibleUnitCount();
        track.resize(unitWidth * editorService.trackWidthInUnits(track.index()), trackArea.height);
        track.x = unitWidth * editorService.trackPositionInUnits(track.index());
        track.y = 0;
    }
    function _connectTrack(track) {
        track.leftClicked.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleLeftClicked(track, columnIndex, lineIndex, x, y);
        });
        track.rightClicked.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleRightClicked(track, columnIndex, lineIndex, x, y);
        });
        track.leftPressed.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleLeftPressed(track, columnIndex, lineIndex, x, y);
        });
        track.leftReleased.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleLeftReleased(track, columnIndex, lineIndex, x, y);
        });
        track.rightPressed.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleRightPressed(track, columnIndex, lineIndex, x, y);
        });
        track.rightReleased.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleRightReleased(track, columnIndex, lineIndex, x, y);
        });
        track.mouseMoved.connect((columnIndex, lineIndex, x, y) => {
            mouseHandler.handleMouseMoved(track, columnIndex, lineIndex, x, y);
        });
    }
    function _createTracks(pattern) {
        pattern.createTracks(positionBar, trackArea.width, trackArea.height);
        pattern.tracks().forEach(track => {
            _connectTrack(track);
        });
    }
    function _createPattern(patternIndex) {
        uiLogger.debug(_tag, `Creating pattern index=${patternIndex}`);
        const pattern = patternComponent.createObject(trackArea);
        pattern.setIndex(patternIndex);
        _createTracks(pattern);
        _patterns.push(pattern);
        mixerService.update();
        noteColumnModelHandler.updatePattern(patternIndex);
    }
    function _createPatterns() {
        uiLogger.info(_tag, `Track count: ${editorService.trackCount()}`);
        uiLogger.debug(_tag, `Editor view width: ${rootItem.width}`);
        for (const patternIndex of editorService.patternIndices()) {
            _createPattern(patternIndex);
        }
    }
    function _recreatePatterns() {
        uiLogger.debug(_tag, `Recreating track layout..`);
        noteColumnModelHandler.clear();
        _clearPatterns();
        _createPatterns();
        _updatePatternVisibility();
    }
    function _changeSong() {
        uiLogger.debug(_tag, `Changing song..`);
        _recreatePatterns();
    }
    function _patternByIndex(index: int): var {
        return _patterns.find(pattern => pattern.index() === index) || null;
    }
    function _currentPattern() {
        return _patternByIndex(editorService.currentPattern);
    }
    function _updateCurrentTrackDimensions() {
        uiLogger.debug(_tag, `Updating current track dimensions of the current pattern..`);
        _currentPattern().updateTrackDimensions(trackArea.width, trackArea.height);
    }
    function _updateAllTrackDimensions() {
        uiLogger.debug(_tag, `Updating track dimensions of all patterns..`);
        _patterns.forEach(pattern => {
            pattern.updateTrackDimensions(trackArea.width, trackArea.height);
        });
    }
    function _updateCurrentTrackData() {
        _currentPattern().updateTrackData();
    }
    function _setTrackFocused(position) {
        _patternByIndex(position.pattern).setTrackFocused(position.track, position.column);
    }
    function _setTrackUnfocused(position) {
        _patternByIndex(position.pattern).setTrackUnfocused(position.track, position.column);
    }
    function _updateColumnHeaders() {
        _patterns.forEach(pattern => pattern.updateColumnHeaders());
    }
    function _updateTrackHeaders() {
        _patterns.forEach(pattern => pattern.updateTrackHeaders());
    }
    function _updateFocus(newPosition, oldPosition) {
        rootItem.focus = true;
        _setTrackUnfocused(oldPosition);
        _setTrackFocused(newPosition);
    }
    function _setPosition(newPosition) {
        _currentPattern().setPosition(newPosition);
        lineNumberColumnLeft.setPosition(newPosition);
        lineNumberColumnRight.setPosition(newPosition);
    }
    function _updatePatternVisibility() {
        const currentPatternIndex = editorService.currentPattern;
        _patterns.forEach(pattern => {
            pattern.visible = pattern.index() === currentPatternIndex;
        });
    }
    function _updateTracksOnHorizontalScroll() {
        _updateCurrentTrackDimensions();
    }
    function _changePattern() {
        _updatePatternVisibility();
        _updateCurrentTrackDimensions();
    }
    function _addColumn(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.addColumn(trackIndex);
        });
        _updateCurrentTrackDimensions();
        noteColumnModelHandler.updateColumns(trackIndex);
    }
    function _addTrack(trackIndex) {
        _patterns.forEach(pattern => {
            const track = pattern.addTrack(trackIndex);
            if (track) {
                _connectTrack(track);
            }
        });
        _updateCurrentTrackDimensions();
    }
    function _deleteColumn(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.deleteColumn(trackIndex);
        });
        _updateCurrentTrackDimensions();
    }
    function _deleteTrack(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.deleteTrack(trackIndex);
        });
        _updateCurrentTrackDimensions();
    }
    function _clearMixerSettings() {
        _patterns.forEach(pattern => {
            pattern.clearMixerSettings();
        });
    }
    function _connectSignals() {
        editorService.columnAdded.connect(trackIndex => _addColumn(trackIndex));
        editorService.columnDeleted.connect(trackIndex => _deleteColumn(trackIndex));
        editorService.columnNameChanged.connect(_updateColumnHeaders);
        editorService.horizontalScrollChanged.connect(_updateTracksOnHorizontalScroll);
        editorService.songChanged.connect(_changeSong);
        editorService.trackAdded.connect(_addTrack);
        editorService.trackDeleted.connect(_deleteTrack);
        editorService.trackNameChanged.connect(_updateTrackHeaders);
        editorService.patternCreated.connect(patternIndex => _createPattern(patternIndex));
        editorService.positionChanged.connect((newPosition, oldPosition) => {
            if (newPosition.pattern !== oldPosition.pattern) {
                uiLogger.debug(_tag, `Changing pattern from index=${oldPosition.pattern} to index=${newPosition.pattern}`);
                _changePattern();
            }
            _updateFocus(newPosition, oldPosition);
            _setPosition(newPosition);
        });
        mixerService.columnMuted.connect((trackIndex, columnIndex, muted) => {
            _patterns.forEach(pattern => {
                pattern.setColumnMuted(trackIndex, columnIndex, muted);
            });
        });
        mixerService.columnSoloed.connect((trackIndex, columnIndex, soloed) => {
            _patterns.forEach(pattern => {
                pattern.setColumnSoloed(trackIndex, columnIndex, soloed);
            });
        });
        mixerService.trackMuted.connect((trackIndex, muted) => {
            _patterns.forEach(pattern => {
                pattern.setTrackMuted(trackIndex, muted);
            });
        });
        mixerService.trackSoloed.connect((trackIndex, soloed) => {
            _patterns.forEach(pattern => {
                pattern.setTrackSoloed(trackIndex, soloed);
            });
        });
        mixerService.columnVelocityScaleChanged.connect((trackIndex, columnIndex, value) => {
            _patterns.forEach(pattern => {
                pattern.setColumnVelocityScale(trackIndex, columnIndex, value);
            });
        });
        mixerService.trackVelocityScaleChanged.connect((trackIndex, value) => {
            _patterns.forEach(pattern => {
                pattern.setTrackVelocityScale(trackIndex, value);
            });
        });
        mixerService.cleared.connect(_clearMixerSettings);
        mouseHandler.editorFocusRequested.connect(forceActiveFocus);
    }
    function _lineNumberColumnHeight() {
        return trackArea.height - Constants.trackHeaderHeight - Constants.columnHeaderHeight;
    }
    function _lineNumberColumnWidth() {
        return Constants.lineNumberColumnWidth;
    }
    function _positionBarHeight() {
        return _lineNumberColumnHeight() / settingsService.visibleLines;
    }
    function _positionBarY() {
        return editorService.positionBarLine() * _positionBarHeight() + Constants.trackHeaderHeight + Constants.columnHeaderHeight;
    }
    function _createLineColumns() {
        lineNumberColumnLeft.width = _lineNumberColumnWidth();
        lineNumberColumnLeft.updateData();
        lineNumberColumnRight.width = _lineNumberColumnWidth();
        lineNumberColumnRight.updateData();
    }
    function _updateLineColumns() {
        lineNumberColumnLeft.resize(_lineNumberColumnWidth(), _lineNumberColumnHeight());
        lineNumberColumnRight.resize(_lineNumberColumnWidth(), _lineNumberColumnHeight());
    }
    Component.onCompleted: initialize()
}
