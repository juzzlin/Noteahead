import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

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
    function clearTracks() {
        _tracks.forEach(track => {
                track.destroy();
            });
        _tracks = [];
    }
    function setTrackDimensionsByIndex(track, trackIndex) {
        track.width = rootItem.width / _trackCount;
        track.height = rootItem.height;
        track.y = 0;
        track.x = trackIndex * track.width;
    }
    function createTracks() {
        _trackCount = editorService.trackCount();
        console.log(`Editor view width: ${rootItem.width}`);
        for (let trackIndex = 0; trackIndex < _trackCount; trackIndex++) {
            const track = trackComponent.createObject(editorView);
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
        updateTrackSizes();
    }
    function updateTrackSizes() {
        _tracks.forEach(track => {
                track.x = track.index() * track.width;
                track.resize(rootItem.width / _trackCount, rootItem.height);
            });
    }
    function connectSignals() {
        editorService.songChanged.connect(refreshTracks);
    }
    function initialize() {
        connectSignals();
        refreshTracks();
    }
    Component.onCompleted: initialize()
}
