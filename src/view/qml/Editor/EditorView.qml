import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
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
    }
    PositionBar {
        id: positionBar
        width: rootItem.width
        height: _positionBarHeight()
        x: 0
        y: _positionBarY()
        z: 10
        border.width: UiService.editMode() ? 2 : 0
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
    ScrollBar {
        id: horizontalScrollBar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Horizontal
        size: editorService.scrollBarSize
        stepSize: editorService.scrollBarStepSize
        snapMode: ScrollBar.SnapAlways
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        onPositionChanged: {
            editorService.requestHorizontalScrollPositionChange(position);
        }
    }
    MainContextMenu {
        id: contextMenu
        width: parent.width * 0.25
        height: parent.height * 0.5
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
        editorService.requestTrackFocus(0, 0, 0);
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _updateTrackSizes();
        _updateLineColumns();
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
        track.leftClicked.connect((columnIndex, lineIndex) => {
            editorService.requestTrackFocus(track.index(), columnIndex, lineIndex);
            rootItem.forceActiveFocus();
        });
        track.rightClicked.connect((columnIndex, lineIndex, x, y) => {
            editorService.requestTrackFocus(track.index(), columnIndex, lineIndex);
            rootItem.forceActiveFocus();
            contextMenu.x = track.x + x;
            contextMenu.y = track.y + y;
            contextMenu.open();
        });
    }
    function _createTracks(pattern) {
        pattern.createTracks(positionBar);
        pattern.tracks().forEach(track => {
            _setTrackDimensions(track);
            _connectTrack(track);
        });
    }
    function _createPattern(patternIndex) {
        uiLogger.debug(_tag, `Creating pattern index=${patternIndex}`);
        const pattern = patternComponent.createObject(trackArea);
        pattern.setIndex(patternIndex);
        _createTracks(pattern);
        pattern.updateIndexHighlights();
        pattern.updateColumnHeaders();
        _patterns.push(pattern);
        mixerService.update();
    }
    function _createPatterns() {
        uiLogger.info(_tag, `Track count: ${editorService.trackCount()}`);
        uiLogger.debug(_tag, `Editor view width: ${rootItem.width}`);
        for (let patternIndex = 0; patternIndex < editorService.patternCount(); patternIndex++) {
            _createPattern(patternIndex);
        }
    }
    function _recreatePatterns() {
        uiLogger.debug(_tag, `Recreating track layout..`);
        _clearPatterns();
        _createPatterns();
        _updatePatternVisibility();
        _updateTrackVisibility();
        _updateIndexHighlights();
    }
    function _updateCurrentTrackDimensions() {
        const currentPattern = _patterns[editorService.currentPattern];
        for (const track of currentPattern._tracks) {
            _setTrackDimensions(track);
        }
    }
    function _updateCurrentTrackData() {
        const currentPattern = _patterns[editorService.currentPattern];
        for (const track of currentPattern._tracks) {
            track.updateData();
        }
    }
    function _setTrackFocused(position) {
        _patterns[position.pattern].setTrackFocused(position.track, position.column);
    }
    function _setTrackUnfocused(position) {
        _patterns[position.pattern].setTrackUnfocused(position.track, position.column);
    }
    function _updateColumnHeaders() {
        _patterns.forEach(pattern => pattern.updateColumnHeaders());
    }
    function _updateTrackHeaders() {
        _patterns.forEach(pattern => pattern.updateTrackHeaders());
    }
    function _updateTrackSizes() {
        const currentPattern = _patterns[editorService.currentPattern];
        currentPattern._tracks.forEach(track => {
            _setTrackDimensions(track);
        });
    }
    function _updateFocus(newPosition, oldPosition) {
        rootItem.focus = true;
        _setTrackUnfocused(oldPosition);
        _setTrackFocused(newPosition);
    }
    function _updatePosition(newPosition) {
        const currentPattern = _patterns[editorService.currentPattern];
        currentPattern._tracks.forEach(track => {
            track.setPosition(newPosition);
        });
        lineNumberColumnLeft.setPosition(newPosition);
        lineNumberColumnRight.setPosition(newPosition);
    }
    function _updateNoteDataAtPosition(position) {
        const currentPattern = _patterns[editorService.currentPattern];
        currentPattern._tracks.forEach(track => {
            track.updateNoteDataAtPosition(position);
        });
    }
    function _updatePatternVisibility() {
        const currentPatternIndex = editorService.currentPattern;
        _patterns.forEach(pattern => {
            pattern.visible = pattern.index() === currentPatternIndex;
        });
    }
    function _updateTrackVisibility() {
        _patterns.forEach(pattern => pattern.updateTrackVisibility());
    }
    function _changePattern() {
        _updatePatternVisibility();
        _updateCurrentTrackDimensions();
    }
    function _updateCurrentLineCount(oldLineCount, newLineCount) {
        const currentPattern = _patterns[editorService.currentPattern];
        _createTracks(currentPattern);
    }
    function _updateIndexHighlights() {
        _patterns.forEach(pattern => pattern.updateIndexHighlights());
    }
    function _addColumn(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.addColumn(trackIndex);
        });
        _updateCurrentTrackDimensions();
    }
    function _deleteColumn(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.deleteColumn(trackIndex);
        });
        _updateCurrentTrackDimensions();
        _updateTrackVisibility();
    }
    function _deleteTrack(trackIndex) {
        _patterns.forEach(pattern => {
            pattern.deleteTrack(trackIndex);
        });
        _updateCurrentTrackDimensions();
        _updateTrackVisibility();
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
        editorService.currentLineCountModified.connect(_updateCurrentLineCount);
        editorService.horizontalScrollChanged.connect(_updateCurrentTrackDimensions);
        editorService.horizontalScrollChanged.connect(_updateTrackVisibility);
        editorService.linesPerBeatChanged.connect(_updateIndexHighlights);
        editorService.lineDataChanged.connect(_updateIndexHighlights);
        editorService.noteDataAtPositionChanged.connect(_updateNoteDataAtPosition);
        editorService.songChanged.connect(_recreatePatterns);
        editorService.trackConfigurationChanged.connect(_recreatePatterns);
        editorService.trackDeleted.connect(_deleteTrack);
        editorService.trackNameChanged.connect(_updateTrackHeaders);
        editorService.patternCreated.connect(patternIndex => _createPattern(patternIndex));
        editorService.positionChanged.connect((newPosition, oldPosition) => {
            if (newPosition.pattern !== oldPosition.pattern) {
                _changePattern();
            }
            _updateFocus(newPosition, oldPosition);
            _updatePosition(newPosition);
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
        mixerService.cleared.connect(_clearMixerSettings);
    }
    function _lineNumberColumnHeight() {
        return trackArea.height - Constants.trackHeaderHeight - Constants.columnHeaderHeight;
    }
    function _lineNumberColumnWidth() {
        return Constants.lineNumberColumnWidth;
    }
    function _positionBarHeight() {
        return _lineNumberColumnHeight() / config.visibleLines;
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
