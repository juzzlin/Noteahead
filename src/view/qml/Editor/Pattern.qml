// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15

Item {
    id: rootItem
    property int _index: 0
    property var _tracks: []
    property var _positionBar
    property bool _dirty: false
    readonly property string _tag: "Pattern"
    function createTracks(positionBar: var, trackAreaWidth: int, trackAreaHeight: int): void {
        _positionBar = positionBar
        _clearTracks();
        const unitWidth = trackAreaWidth / editorService.visibleUnitCount();
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
                track.setDimensions(trackAreaWidth, trackAreaHeight, unitWidth);
                _tracks.push(track);
                uiLogger.trace(_tag, `Added track index=${trackIndex}, width=${track.width}, height=${track.height}, x=${track.x}, y=${track.y}`);
            }
        }
        updateColumnHeaders();
    }
    function _clearTracks(): void {
        _tracks.forEach(track => track.destroy());
        _tracks.length = 0;
    }
    function index(): int {
        return _index;
    }
    function setIndex(index: int): void {
        _index = index;
    }
    function updateLocation(newPatternIndex: int): void {
        _index = newPatternIndex;
        _tracks.forEach(track => {
            track.setLocation(_index, track.index());
        });
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
            if (editorService.isTrackVisible(track.index())) {
                track.setPosition(newPosition);
            }
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
