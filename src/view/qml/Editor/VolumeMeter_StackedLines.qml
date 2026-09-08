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

// A ladder of thin horizontal lines in the accent colour, in the same footprint as the gradient
// bar. The rungs are built once and revealed by a clip that follows the level, so playback animates
// a single height and never creates items. See VolumeMeter.qml for the animation that drives it.
Item {
    id: rootItem
    property real level: 0.0 // Normalized level (0.0 to 1.0)
    property real maxHeight: 1.0
    readonly property int _lineHeight: 2
    readonly property int _lineSpacing: 3
    readonly property int _pitch: _lineHeight + _lineSpacing
    readonly property real _ladderHeight: height * maxHeight
    Item {
        id: levelIndicatorClip
        width: parent.width * 0.2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        // Snapped to whole rungs, so a line lights up as a unit instead of being sliced. Anything
        // audible lights at least one rung.
        height: rootItem.level > 0 ? rootItem._pitch * Math.max(1, Math.floor(rootItem._ladderHeight * rootItem.level / rootItem._pitch)) : 0
        clip: true
        Column {
            id: lines
            width: parent.width
            anchors.bottom: parent.bottom
            spacing: rootItem._lineSpacing
            Repeater {
                model: Math.max(1, Math.floor(rootItem._ladderHeight / rootItem._pitch))
                Rectangle {
                    width: lines.width
                    height: rootItem._lineHeight
                    color: themeService.accentColor
                }
            }
        }
    }
}
