import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

FocusScope {
    id: rootItem
    property int _trackCount: 0
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
        anchors.topMargin: Constants.trackHeaderHeight
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
        anchors.topMargin: Constants.trackHeaderHeight
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
        _patterns = [];
    }
    function _setTrackDimensionsByIndex(track, trackIndex) {
        const unitWidth = trackArea.width / editorService.visibleUnitCount();
        track.resize(unitWidth * editorService.trackWidthInUnits(trackIndex), trackArea.height);
        track.x = unitWidth * editorService.trackPositionInUnits(trackIndex);
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
                _setTrackDimensionsByIndex(track, track.index());
                _connectTrack(track);
            });
    }
    function _createPattern(patternIndex) {
        uiLogger.debug(_tag, `Creating pattern index=${patternIndex}`);
        const pattern = patternComponent.createObject(trackArea);
        pattern.setIndex(patternIndex);
        _createTracks(pattern);
        _patterns.push(pattern);
    }
    function _createPatterns() {
        _trackCount = editorService.trackCount();
        uiLogger.debug(_tag, `Editor view width: ${rootItem.width}`);
        for (let patternIndex = 0; patternIndex < editorService.patternCount(); patternIndex++) {
            uiLogger.debug(_tag, `Creating pattern index=${patternIndex}`);
            _createPattern(patternIndex);
        }
    }
    function _recreatePatterns() {
        uiLogger.debug(_tag, `Recreating track layout..`);
        for (const pattern of _patterns) {
            pattern.clearTracks();
        }
        _clearPatterns();
        _createPatterns();
        _updatePatternVisibility();
        _updateTrackVisibility();
    }
    function _updateCurrentTrackDimensions() {
        const currentPattern = _patterns[editorService.currentPattern];
        for (const track of currentPattern._tracks) {
            _setTrackDimensionsByIndex(track, track.index());
        }
    }
    function _updateCurrentTrackData() {
        const currentPattern = _patterns[editorService.currentPattern];
        for (const track of currentPattern._tracks) {
            track.updateData();
        }
    }
    function _setTrackFocused(position) {
        _patterns[position.pattern]._tracks[position.track].setFocused(position.column, true);
    }
    function _setTrackUnfocused(position) {
        _patterns[position.pattern]._tracks[position.track].setFocused(position.column, false);
    }
    function _updateTrackHeaders() {
        _patterns.forEach(pattern => pattern.updateTrackHeaders());
    }
    function _updateTrackSizes() {
        const currentPattern = _patterns[editorService.currentPattern];
        currentPattern._tracks.forEach(track => {
                _setTrackDimensionsByIndex(track, track.index());
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
    }
    function _connectSignals() {
        editorService.columnAdded.connect(trackIndex => _addColumn(trackIndex));
        editorService.columnDeleted.connect(trackIndex => _deleteColumn(trackIndex));
        editorService.currentLineCountModified.connect(_updateCurrentLineCount);
        editorService.horizontalScrollChanged.connect(_updateCurrentTrackDimensions);
        editorService.horizontalScrollChanged.connect(_updateTrackVisibility);
        editorService.noteDataAtPositionChanged.connect(_updateNoteDataAtPosition);
        editorService.songChanged.connect(_recreatePatterns);
        editorService.trackConfigurationChanged.connect(_recreatePatterns);
        editorService.trackNameChanged.connect(_updateTrackHeaders);
        editorService.patternCreated.connect(patternIndex => _createPattern(patternIndex));
        editorService.positionChanged.connect((newPosition, oldPosition) => {
                if (newPosition.pattern !== oldPosition.pattern) {
                    _changePattern();
                }
                _updateFocus(newPosition, oldPosition);
                _updatePosition(newPosition);
            });
    }
    function _lineNumberColumnHeight() {
        return trackArea.height - Constants.trackHeaderHeight;
    }
    function _lineNumberColumnWidth() {
        return Constants.lineNumberColumnWidth;
    }
    function _positionBarHeight() {
        return _lineNumberColumnHeight() / config.visibleLines;
    }
    function _positionBarY() {
        return editorService.positionBarLine() * _positionBarHeight() + Constants.trackHeaderHeight;
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
