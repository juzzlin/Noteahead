import QtQuick 2.15

Item {
    id: rootItem
    property int _index: 0
    property var _tracks: []
    readonly property string _tag: "Pattern"
    function createTracks(positionBar) {
        _tracks.length = 0;
        for (let trackIndex of editorService.trackIndices()) {
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
        const track = trackByIndex(trackIndex);
        if (track) {
            track.addColumn();
        }
    }
    function deleteColumn(trackIndex) {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.deleteColumn();
        }
    }
    function deleteTrack(trackIndex) {
        const track = trackByIndex(trackIndex);
        if (track) {
            _tracks = _tracks.filter(t => t !== track); // Remove the track from the array
            track.destroy(); // Destroy the track object
            uiLogger.debug(_tag, `Deleted track index=${trackIndex}`);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function index() {
        return _index;
    }
    function setIndex(index) {
        _index = index;
    }
    function setTrackFocused(trackIndex, column) {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.setFocused(column, true);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function setTrackUnfocused(trackIndex, column) {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.setFocused(column, false);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function setColumnMuted(trackIndex, columnIndex, muted) {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Muting column ${columnIndex} of track ${trackIndex}: ${muted}`);
                track.setColumnMuted(columnIndex, muted);
            }
        });
    }
    function setColumnSoloed(trackIndex, columnIndex, soloed) {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Soloing column ${columnIndex} of track ${trackIndex}: ${soloed}`);
                track.setColumnSoloed(columnIndex, soloed);
            }
        });
    }
    function setTrackMuted(trackIndex, muted) {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Muting track ${trackIndex}: ${muted}`);
                track.setMuted(muted);
            }
        });
    }
    function setTrackSoloed(trackIndex, soloed) {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Soloing track ${trackIndex}: ${soloed}`);
                track.setSoloed(soloed);
            }
        });
    }
    function clearMixerSettings() {
        _tracks.forEach(track => {
            track.clearMixerSettings();
        });
    }
    function tracks() {
        return _tracks;
    }
    function trackByIndex(trackIndex) {
        return _tracks.find(track => track.index() === trackIndex) || null;
    }
    function updateIndexHighlights() {
        _tracks.forEach(track => {
            track.updateIndexHighlights();
        });
    }
    function updateTrackHeaders() {
        _tracks.forEach(track => {
            track.setName(editorService.trackName(track.index()));
        });
    }
    function updateTrackVisibility() {
        _tracks.forEach(track => track.updateColumnVisibility());
    }
    Component {
        id: trackComponent
        Track {
            height: rootItem.height
        }
    }
}
