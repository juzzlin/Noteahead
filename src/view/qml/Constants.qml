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

pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property int columnHeaderHeight: 40
    readonly property double defaultWindowScale: 0.8
    readonly property double defaultDialogScale: 0.7
    readonly property double effectDialogScale: 0.5
    readonly property double largeDialogScale: 0.85
    readonly property int minWindowWidth: 1024
    readonly property int minWindowHeight: 768

    // The editor fits as many note columns side by side as it can give this much width to. Below
    // minUnitWidth the track headers get cramped, so a narrow window shows fewer and a wide one
    // shows more; the ceiling is whatever the song actually has. defaultVisibleUnitCount is what a
    // fixed, non-fitting editor shows.
    readonly property int minUnitWidth: 280
    readonly property int defaultVisibleUnitCount: 6
    readonly property int minVisibleUnitCount: 2

    // Anything shorter and the 88 keys are not worth clicking
    readonly property int minKeyboardHeight: 60

    readonly property int lineNumberColumnWidth: 50
    readonly property int mainToolBarButtonSize: 32
    readonly property int defaultButtonWidth: 100
    readonly property int toolTipDelay: 1000
    readonly property int toolTipTimeout: 5000
    readonly property int trackBorderWidth: 1
    readonly property int trackBorderFocusedWidth: 2
    readonly property int trackHeaderHeight: 40

    readonly property int transposeMin: -48
    readonly property int transposeMax: 48

    readonly property int positionBarBorderWidth: 1
    readonly property double positionBarOpacity: 0.25

    readonly property real maxEventDelay: 10000

    readonly property int dialogEnterTransitionDuration: 120
    readonly property int dialogExitTransitionDuration: 200
    readonly property real dialogExitScale: 0.85

    readonly property real uiInternalScaling: 1000.0
}
