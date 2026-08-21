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

// The guidance shown in the left half of the bottom bar. One fixed tip per context: it changes
// when the state changes and at no other time, deliberately. A tip that rotated on a timer would
// keep moving in the corner of the eye of someone trying to work.
//
// This is a binding, not a queue, which is the whole point: the tip is simply true at all times
// rather than being pushed once at startup and flushed by the next status message.
QtObject {
    readonly property string currentTip: applicationService.editMode ? qsTr("<b>Z–M</b> and <b>Q–U</b> play notes, <b>F3</b>/<b>F4</b> change octave, <b>A</b> inserts a note off") : qsTr("Press <b>ESC</b> to edit, <b>SPACE</b> to play, letter keys are notes")
}
