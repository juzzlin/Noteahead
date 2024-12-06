import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

Item {
    id: rootItem

    property int _trackCount: 0

    property var _trackComponents: []

    Component {
        id: trackComponent
        Track {
            height: rootItem.height
        }
    }

    function clearTracks() {
        _trackComponents.forEach(track => {
                track.destroy();
            });
        _trackComponents = [];
    }

    function setTrackDimensionsByIndex(track, trackIndex) {
        track.width = rootItem.width / _trackCount;
        track.x = trackIndex * track.width;
    }

    function createTracks() {
        _trackCount = editorService.trackCount();
        console.log(`Editor view width: ${rootItem.width}`);
        for (let trackIndex = 0; trackIndex < _trackCount; trackIndex++) {
            const track = trackComponent.createObject(editorView, {
                    "index": trackIndex
                });
            if (track) {
                setTrackDimensionsByIndex(track, trackIndex);
                _trackComponents.push(track);
                track.setName(editorService.trackName(trackIndex));
                track.nameChanged.connect(name => editorService.setTrackName(trackIndex, name));
                console.log(`Added track index=${trackIndex}, width=${track.width}, x=${track.x}`);
            }
        }
    }

    function refreshTracks() {
        console.log(`Refreshing track layout..`);
        clearTracks();
        createTracks();
    }

    function updateTrackSizes() {
        for (let i = 0; i < _trackComponents.length; i++) {
            setTrackDimensionsByIndex(_trackComponents[i], i);
        }
    }

    function connectSignals() {
        editorService.songChanged.connect(refreshTracks);
    }

    function initialize() {
        connectSignals();
        refreshTracks();
    }

    Component.onCompleted: initialize()

    onWidthChanged: updateTrackSizes()

    onHeightChanged: updateTrackSizes()
}
