import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    property int _trackCount: 0
    property var _tracks: []
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
    function _updateTrackSizes() {
        _tracks.forEach(track => {
                track.resize(trackArea.width / _trackCount, trackArea.height);
                track.x = track.index() * track.width;
            });
    }
    function connectSignals() {
        editorService.songChanged.connect(refreshTracks);
    }
    function initialize() {
        connectSignals();
        refreshTracks();
        _createLineColumns();
    }
    function _lineNumberColumnHeight() {
        return trackArea.height - Constants.trackHeaderHeight;
    }
    function _lineNumberColumnWidth() {
        return Constants.lineNumberColumnWidth;
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
