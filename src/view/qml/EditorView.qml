import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: rootItem

    property int _trackCount: 0
    property var _trackComponents: []

    Component {
        id: trackComponent
        Rectangle {
            height: rootItem.height
            color: index % 2 === 0 ? "lightgray" : "darkgray"
            border.color: "black"
            border.width: 1

            property int index: 0

            Text {
                anchors.centerIn: parent
                text: "Track " + (parent.index + 1)
                color: "black"
            }
        }
    }

    function initialize() {
        connectSignals();
    }

    function connectSignals() {
        editorService.songChanged.connect(refreshTracks);
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

    onWidthChanged: updateTrackSizes()
    onHeightChanged: updateTrackSizes()
}
