import QtQuick 2.15

Item {
    id: rootItem
    property int _index: 0
    property var _tracks: []
    readonly property string _tag: "Pattern"
    function createTracks(positionBar: var, trackAreaWidth: int, trackAreaHeight: int): void {
        _tracks.length = 0;
        for (let trackIndex of editorService.trackIndices()) {
            const track = trackComponent.createObject(this);
            if (track) {
                track.setLocation(_index, trackIndex);
                track.setName(editorService.trackName(trackIndex));
                track.setPositionBar(positionBar);
                track.nameChanged.connect(name => {
                    editorService.setTrackName(trackIndex, name);
                });
                track.updateData();
                _tracks.push(track);
                uiLogger.trace(_tag, `Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
        updateTrackDimensions(trackAreaWidth, trackAreaHeight);
        updateColumnHeaders();
    }
    function addColumn(trackIndex: int): void {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.addColumn();
        }
    }
    function deleteColumn(trackIndex: int): void {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.deleteColumn();
        }
    }
    function deleteTrack(trackIndex: int): void {
        const track = trackByIndex(trackIndex);
        if (track) {
            _tracks = _tracks.filter(t => t !== track); // Remove the track from the array
            track.destroy(); // Destroy the track object
            uiLogger.debug(_tag, `Deleted track index=${trackIndex}`);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function index(): int {
        return _index;
    }
    function setIndex(index: int): void {
        _index = index;
    }
    function setTrackFocused(trackIndex: int, column: int): void {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.setFocused(column, true);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function setTrackUnfocused(trackIndex: int, column: int): void {
        const track = trackByIndex(trackIndex);
        if (track) {
            track.setFocused(column, false);
        } else {
            uiLogger.error(_tag, `No such track: index=${trackIndex}`);
        }
    }
    function setColumnMuted(trackIndex: int, columnIndex: int, muted: bool): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Muting column ${columnIndex} of track ${trackIndex}: ${muted}`);
                track.setColumnMuted(columnIndex, muted);
            }
        });
    }
    function setColumnSoloed(trackIndex: int, columnIndex: int, soloed: bool): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Soloing column ${columnIndex} of track ${trackIndex}: ${soloed}`);
                track.setColumnSoloed(columnIndex, soloed);
            }
        });
    }
    function setColumnVelocityScale(trackIndex: int, columnIndex: int, value: int): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.trace(_tag, `Setting velocity scale for column ${columnIndex} of track ${trackIndex}: ${value}`);
                track.setColumnVelocityScale(columnIndex, value);
            }
        });
    }
    function setTrackMuted(trackIndex: int, muted: bool): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Muting track ${trackIndex}: ${muted}`);
                track.setMuted(muted);
            }
        });
    }
    function setTrackSoloed(trackIndex: int, soloed: bool): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.debug(_tag, `Soloing track ${trackIndex}: ${soloed}`);
                track.setSoloed(soloed);
            }
        });
    }
    function setTrackVelocityScale(trackIndex: int, value: int): void {
        _tracks.forEach(track => {
            if (track.index() === trackIndex) {
                uiLogger.trace(_tag, `Setting velocity scale for track ${trackIndex}: ${value}`);
                track.setVelocityScale(value);
            }
        });
    }
    function setPosition(newPosition: var): void {
        _tracks.forEach(track => {
            track.setPosition(newPosition);
        });
    }
    function clearMixerSettings(): void {
        _tracks.forEach(track => track.clearMixerSettings());
    }
    function tracks(): var {
        return _tracks;
    }
    function trackByIndex(trackIndex: int): var {
        return _tracks.find(track => track.index() === trackIndex) || null;
    }
    function updateColumnHeaders(): void {
        _tracks.forEach(track => track.updateColumnHeaders());
    }
    function updateTrackHeaders(): void {
        _tracks.forEach(track => track.setName(editorService.trackName(track.index())));
    }
    function updateTrackData(): void {
        _tracks.forEach(track => track.updateData());
    }
    function updateTrackDimensions(trackAreaWidth: int, trackAreaHeight: int): void {
        const unitWidth = trackAreaWidth / editorService.visibleUnitCount();
        _tracks.forEach(track => track.setDimensions(trackAreaWidth, trackAreaHeight, unitWidth));
    }
    Component {
        id: trackComponent
        Track {
            height: rootItem.height
        }
    }
}
