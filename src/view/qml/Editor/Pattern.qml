import QtQuick 2.15

Item {
    id: rootItem
    property int _index: 0
    property var _tracks: []
    readonly property string _tag: "Pattern"
    function clearTracks() {
        _tracks = [];
    }
    function createTracks(positionBar) {
        _tracks = [];
        for (let trackIndex = 0; trackIndex < _trackCount; trackIndex++) {
            const track = trackComponent.createObject(this);
            if (track) {
                track.setIndex(trackIndex);
                track.setPatternIndex(_index);
                track.setName(editorService.trackName(trackIndex));
                track.setPositionBar(positionBar);
                track.nameChanged.connect(name => {
                        editorService.setTrackName(trackIndex, name);
                    });
                track.updateData();
                _tracks.push(track);
                uiLogger.debug(_tag, `Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
    }
    function addColumn(trackIndex) {
        _tracks[trackIndex].addColumn();
    }
    function deleteColumn(trackIndex) {
        _tracks[trackIndex].deleteColumn();
    }
    function index() {
        return _index;
    }
    function setIndex(index) {
        _index = index;
    }
    function tracks() {
        return _tracks;
    }
    function updateTrackHeaders() {
        _tracks.forEach(track => {
                track.setName(editorService.trackName(track.index()));
            });
    }
    Component {
        id: trackComponent
        Track {
            height: rootItem.height
        }
    }
}
