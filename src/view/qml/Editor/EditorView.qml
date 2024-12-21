import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

FocusScope {
    id: rootItem
    property int _trackCount: 0
    property var _tracks: []
    readonly property string _tag: "EditorView"
    focus: true
    Component {
        id: trackComponent
        Track {
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
    Keys.onPressed: event => {
        keyboardHandler.handleEvent(event);
        event.accepted = true;
    }
    function clearTracks() {
        _tracks.forEach(track => {
            track.destroy();
        });
        _tracks = [];
    }
    function setTrackDimensionsByIndex(track, trackIndex) {
        track.width = trackArea.width / _trackCount;
        track.height = trackArea.height;
        track.y = 0;
        track.x = trackIndex * track.width;
    }
    function _connectTrack(track) {
        track.clicked.connect(() => {
            editorService.requestTrackFocus(track.index());
            rootItem.forceActiveFocus();
        });
    }
    function createTracks() {
        _trackCount = editorService.trackCount();
        uiLogger.debug(_tag, `Editor view width: ${rootItem.width}`);
        for (let trackIndex = 0; trackIndex < _trackCount; trackIndex++) {
            const track = trackComponent.createObject(trackArea);
            if (track) {
                setTrackDimensionsByIndex(track, trackIndex);
                track.setIndex(trackIndex);
                track.setName(editorService.trackName(trackIndex));
                track.nameChanged.connect(name => editorService.setTrackName(trackIndex, name));
                track.updateData();
                _tracks.push(track);
                _connectTrack(track);
                uiLogger.debug(_tag, `Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
    }
    function refreshTracks() {
        uiLogger.debug(_tag, `Refreshing track layout..`);
        clearTracks();
        createTracks();
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _updateTrackSizes();
        _updateLineColumns();
    }
    function _setTrackFocused(trackIndex) {
        _tracks[trackIndex].setFocused(true);
    }
    function _setTrackUnfocused(trackIndex) {
        _tracks[trackIndex].setFocused(false);
    }
    function _updateTrackSizes() {
        _tracks.forEach(track => {
            track.resize(trackArea.width / _trackCount, trackArea.height);
            track.x = track.index() * track.width;
        });
    }
    function _updateFocus(newPosition, oldPosition) {
        _setTrackUnfocused(oldPosition.track);
        _setTrackFocused(newPosition.track);
    }
    function _updatePosition(newPosition) {
        _tracks.forEach(track => {
            track.setPosition(newPosition);
        });
        lineNumberColumnLeft.setPosition(newPosition);
        lineNumberColumnRight.setPosition(newPosition);
    }
    function _updateNoteDataAtPosition(position) {
        _tracks.forEach(track => {
            track.updateNoteDataAtPosition(position);
        });
    }
    function connectSignals() {
        editorService.noteDataAtPositionChanged.connect(_updateNoteDataAtPosition);
        editorService.songChanged.connect(refreshTracks);
        editorService.positionChanged.connect((newPosition, oldPosition) => {
            _updateFocus(newPosition, oldPosition);
            _updatePosition(newPosition);
        });
    }
    function initialize() {
        connectSignals();
        refreshTracks();
        _createLineColumns();
        editorService.requestTrackFocus(0);
    }
    function _lineNumberColumnHeight() {
        return trackArea.height - Constants.trackHeaderHeight;
    }
    function _lineNumberColumnWidth() {
        return Constants.lineNumberColumnWidth;
    }
    function _positionBarHeight() {
        return _lineNumberColumnHeight() / editorService.linesVisible();
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
