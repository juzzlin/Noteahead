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

// The per-column volume meter: owns the level and its falling animation, and loads the style the
// user picked. Only the chosen style exists, so the one not in use costs nothing.
Item {
    id: rootItem
    property real _volume: 0.0 // Normalized volume (0.0 to 1.0)
    property real _animatedVolume: 0.0
    property real _timeStep: 0.0
    readonly property real _maxHeight: 0.8
    readonly property real _fallingScale: 0.001
    Loader {
        anchors.fill: parent
        // Constants::VolumeMeterStyle::Lines
        sourceComponent: themeService.volumeMeterStyle === 1 ? stackedLinesComponent : gradientBarComponent
    }
    Component {
        id: gradientBarComponent
        VolumeMeter_GradientBar {
            level: rootItem._animatedVolume
            maxHeight: rootItem._maxHeight
        }
    }
    Component {
        id: stackedLinesComponent
        VolumeMeter_StackedLines {
            level: rootItem._animatedVolume
            maxHeight: rootItem._maxHeight
        }
    }
    Timer {
        id: fallTimer
        interval: 17 // ~60 FPS
        repeat: true
        running: false
        onTriggered: {
            if (_animatedVolume > 0) {
                _animatedVolume -= _timeStep * _timeStep;
                _timeStep += interval * _fallingScale;
            } else {
                running = false;
            }
        }
    }
    function trigger(normalizedVolume: double): void {
        if (normalizedVolume >= 0.0 && normalizedVolume <= 1.0) {
            _volume = normalizedVolume;
            if (_volume > 0) {
                _animatedVolume = _volume;
                _timeStep = 0;
                fallTimer.restart();
            }
        }
    }
}
