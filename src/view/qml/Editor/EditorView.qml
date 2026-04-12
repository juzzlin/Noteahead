import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

FocusScope {
    id: rootItem
    property var _activePattern: null
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
    function deleteUnusedPatterns() {
        uiLogger.info(_tag, "Deleting unused patterns..");
        editorService.deleteUnusedPatterns(); // Finalize deletion in C++ side
        _refreshLayout();
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
        if (_activePattern) {
            _activePattern.destroy();
            _activePattern = null;
        }
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
    function _ensureActivePattern() {
        if (!_activePattern) {
            _activePattern = patternComponent.createObject(trackArea);
            _activePattern.setIndex(editorService.currentPattern);
            _createTracks(_activePattern);
        }
        return _activePattern;
    }
    function _recreatePatterns() {
        uiLogger.debug(_tag, `Recreating track layout..`);
        noteColumnModelHandler.clear();
        _clearPatterns();

        uiLogger.info(_tag, `Track count: ${editorService.trackCount()}`);
        uiLogger.debug(_tag, `Editor view width: ${rootItem.width}`);

        _ensureActivePattern();
        mixerService.update();
        const currentPos = editorService.position;
        _setPosition(currentPos);
        _updateFocus(currentPos, currentPos);
        _updateCurrentTrackDimensions();
        editorService.resetModified();
    }
    function _changeSong() {
        uiLogger.debug(_tag, `Changing song..`);
        _recreatePatterns();
    }
    function _patternByIndex(index: int): var {
        const pattern = _ensureActivePattern();
        if (pattern.index() !== index) {
            pattern.updateLocation(index);
        }
        return pattern;
    }
    function _currentPattern() {
        return _patternByIndex(editorService.currentPattern);
    }

    function _updateCurrentTrackDimensions() {
        uiLogger.debug(_tag, `Updating current track dimensions of the current pattern..`);
        const pattern = _currentPattern();
        if (pattern) {
            pattern.updateTrackDimensions(trackArea.width, trackArea.height);
            _setPosition(editorService.position);
        }
    }
    function _updateAllTrackDimensions() {
        uiLogger.debug(_tag, `Updating track dimensions..`);
        const pattern = _currentPattern();
        if (pattern) {
            pattern.updateTrackDimensions(trackArea.width, trackArea.height);
        }
    }
    function _updateCurrentTrackData() {
        const pattern = _currentPattern();
        if (pattern) {
            pattern.updateTrackData();
        }
    }
    function _setTrackFocused(position) {
        const pattern = _patternByIndex(position.pattern);
        if (pattern) {
            pattern.setTrackFocused(position.track, position.column);
        }
    }
    function _setTrackUnfocused(position) {
        const pattern = _patternByIndex(position.pattern);
        if (pattern) {
            pattern.setTrackUnfocused(position.track, position.column);
        }
    }
    function _updateColumnHeaders() {
        const pattern = _currentPattern();
        if (pattern) {
            pattern.updateColumnHeaders();
        }
    }
    function _updateTrackHeaders() {
        const pattern = _currentPattern();
        if (pattern) {
            pattern.updateTrackHeaders();
        }
    }
    function _updateFocus(newPosition, oldPosition) {
        rootItem.focus = true;
        _setTrackUnfocused(oldPosition);
        _setTrackFocused(newPosition);
    }
    function _setPosition(newPosition) {
        const pattern = _currentPattern();
        if (pattern) {
            pattern.setPosition(newPosition);
        }
        lineNumberColumnLeft.setPosition(newPosition);
        lineNumberColumnRight.setPosition(newPosition);
    }
    function _refreshPattern(pattern) {
        uiLogger.debug(_tag, `Refreshing pattern index=${pattern.index()}`);
        _createTracks(pattern);
        pattern.updateTrackDimensions(trackArea.width, trackArea.height);
        _setPosition(editorService.position);
    }
    function _updateTracksOnHorizontalScroll() {
        _updateCurrentTrackDimensions();
    }
    function _changePattern() {
        _updateCurrentTrackDimensions();
    }
    function _refreshLayout() {
        uiLogger.debug(_tag, "Refreshing layout..");
        noteColumnModelHandler.clear();
        const pattern = _currentPattern();
        _refreshPattern(pattern);
        _updateFocus(editorService.position, editorService.position);
    }
    function _clearMixerSettings() {
        const pattern = _currentPattern();
        if (pattern) {
            pattern.clearMixerSettings();
        }
    }
    function _connectSignals() {
        editorService.columnAdded.connect(track => _refreshLayout());
        editorService.columnDeleted.connect(track => _refreshLayout());
        editorService.columnNameChanged.connect(_updateColumnHeaders);
        editorService.horizontalScrollChanged.connect(_updateTracksOnHorizontalScroll);
        editorService.songChanged.connect(_changeSong);
        editorService.trackAdded.connect(track => _refreshLayout());
        editorService.trackDeleted.connect(track => _refreshLayout());
        editorService.trackNameChanged.connect(_updateTrackHeaders);
        editorService.patternCreated.connect(patternIndex => _refreshLayout());
        editorService.positionChanged.connect((newPosition, oldPosition) => {
            if (newPosition.pattern !== oldPosition.pattern) {
                uiLogger.debug(_tag, `Changing pattern from index=${oldPosition.pattern} to index=${newPosition.pattern}`);
                _changePattern();
            }
            _updateFocus(newPosition, oldPosition);
            _setPosition(newPosition);
        });
        mixerService.columnMuted.connect((trackIndex, columnIndex, muted) => {
            if (_activePattern) {
                _activePattern.setColumnMuted(trackIndex, columnIndex, muted);
            }
        });
        mixerService.columnSoloed.connect((trackIndex, columnIndex, soloed) => {
            if (_activePattern) {
                _activePattern.setColumnSoloed(trackIndex, columnIndex, soloed);
            }
        });
        mixerService.trackMuted.connect((trackIndex, muted) => {
            if (_activePattern) {
                _activePattern.setTrackMuted(trackIndex, muted);
            }
        });
        mixerService.trackSoloed.connect((trackIndex, soloed) => {
            if (_activePattern) {
                _activePattern.setTrackSoloed(trackIndex, soloed);
            }
        });
        mixerService.columnVelocityScaleChanged.connect((trackIndex, columnIndex, value) => {
            if (_activePattern) {
                _activePattern.setColumnVelocityScale(trackIndex, columnIndex, value);
            }
        });
        mixerService.trackVelocityScaleChanged.connect((trackIndex, value) => {
            if (_activePattern) {
                _activePattern.setTrackVelocityScale(trackIndex, value);
            }
        });
        mixerService.cleared.connect(_clearMixerSettings);
        UiService.deleteUnusedPatternsConfirmed.connect(deleteUnusedPatterns);
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
