import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
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
            editorService.requestUnitCursorPosition(position);
        }
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
        const unitWidth = trackArea.width / editorService.visibleUnitCount();
        track.resize(unitWidth * editorService.trackWidthInUnits(trackIndex), trackArea.height);
        track.x = unitWidth * editorService.trackPositionInUnits(trackIndex);
        track.y = 0;
    }
    function _connectTrack(track) {
        track.clicked.connect(columnIndex => {
            editorService.requestTrackFocus(track.index(), columnIndex);
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
                track.setPositionBar(positionBar);
                track.nameChanged.connect(name => editorService.setTrackName(trackIndex, name));
                track.updateData();
                _tracks.push(track);
                _connectTrack(track);
                uiLogger.debug(_tag, `Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
    }
    function recreateTracks() {
        uiLogger.debug(_tag, `Recreating track layout..`);
        clearTracks();
        createTracks();
    }
    function refreshTracks() {
        uiLogger.debug(_tag, `Refresing track layout..`);
        for (const track of _tracks) {
            setTrackDimensionsByIndex(track, track.index());
            track.updateData();
            uiLogger.debug(_tag, `Updated track index=${track.index()}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
        }
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _updateTrackSizes();
        _updateLineColumns();
    }
    function _setTrackFocused(trackIndex, columnIndex) {
        _tracks[trackIndex].setFocused(columnIndex, true);
    }
    function _setTrackUnfocused(trackIndex, columnIndex) {
        _tracks[trackIndex].setFocused(columnIndex, false);
    }
    function _updateTrackSizes() {
        _tracks.forEach(track => {
            setTrackDimensionsByIndex(track, track.index());
        });
    }
    function _updateFocus(newPosition, oldPosition) {
        _setTrackUnfocused(oldPosition.track, oldPosition.column);
        _setTrackFocused(newPosition.track, newPosition.column);
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
        editorService.horizontalScrollChanged.connect(refreshTracks);
        editorService.noteDataAtPositionChanged.connect(_updateNoteDataAtPosition);
        editorService.songChanged.connect(recreateTracks);
        editorService.trackConfigurationChanged.connect(recreateTracks);
        editorService.positionChanged.connect((newPosition, oldPosition) => {
            _updateFocus(newPosition, oldPosition);
            _updatePosition(newPosition);
        });
    }
    function initialize() {
        connectSignals();
        recreateTracks();
        _createLineColumns();
        editorService.requestTrackFocus(0, 0);
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
