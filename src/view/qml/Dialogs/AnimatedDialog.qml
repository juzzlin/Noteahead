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
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

// Dialog with a shared open/close transition used as the base for all Noteahead dialogs.
// Opens with a subtle scale-up fade-in and closes with a scale-down fade-out.
Dialog {
    id: root
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: Constants.dialogEnterTransitionDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: Constants.dialogExitScale
            to: 1.0
            duration: Constants.dialogEnterTransitionDuration
            easing.type: Easing.OutCubic
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: Constants.dialogExitTransitionDuration
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: Constants.dialogExitScale
            duration: Constants.dialogExitTransitionDuration
            easing.type: Easing.InCubic
        }
    }
}
