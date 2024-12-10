import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    property int _trackCount: 0
    property var _tracks: []
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
    Item {
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
    }
    LineNumberColumn {
        id: lineNumberColumnRight
        height: _lineNumberColumnHeight()
        width: _lineNumberColumnWidth()
        anchors.top: parent.top
        anchors.topMargin: Constants.trackHeaderHeight
        anchors.right: parent.right
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
            });
    }
    function createTracks() {
        _trackCount = editorService.trackCount();
        console.log(`Editor view width: ${rootItem.width}`);
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
                console.log(`Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
    }
    function refreshTracks() {
        console.log(`Refreshing track layout..`);
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
        console.log(`Setting track ${trackIndex} focused`);
        _tracks.forEach(track => track.setFocused(false));
        _tracks[trackIndex].setFocused(true);
    }
    function _setAllTracksUnfocused() {
        _tracks.forEach(track => track.setFocused(false));
    }
    function _updateTrackSizes() {
        _tracks.forEach(track => {
                track.resize(trackArea.width / _trackCount, trackArea.height);
                track.x = track.index() * track.width;
            });
    }
    function connectSignals() {
        editorService.songChanged.connect(refreshTracks);
        editorService.positionChanged.connect(position => {
                _setTrackFocused(position.track);
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
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Up) {
            editorService.scroll(-1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            editorService.scroll(1);
            event.accepted = true;
        }
    }
    Component.onCompleted: initialize()
}
