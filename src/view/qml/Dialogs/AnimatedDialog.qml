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
import Noteahead 1.0

// Dialog with a shared open/close transition used as the base for all Noteahead dialogs.
// Opens with a subtle scale-up fade-in and closes with a scale-down fade-out.
Dialog {
    id: root

    // Qt 6.4's overlay eats every wheel event aimed at anything that isn't inside the bottom-most
    // open modal popup (QQuickOverlay::eventFilter, fixed after 6.4.2), which leaves the knobs of a
    // dialog opened on top of another dialog deaf to the mouse wheel. Only the topmost dialog needs
    // modality -- it already blocks everything below it -- so a covered dialog gives its own up
    // until it is uncovered again. The dimmer is set here instead of being left to follow modality,
    // so that nothing looks different while the modality is off.
    dim: true

    // A plain onVisibleChanged would be replaced by any dialog that declares one of its own
    Connections {
        target: root
        function onVisibleChanged() {
            if (root.visible) {
                UiService.pushDialog(root);
            } else {
                UiService.popDialog(root);
            }
        }
    }

    Binding {
        target: root
        property: "modal"
        value: false
        when: root.visible && UiService.topDialog !== root
        restoreMode: Binding.RestoreBindingOrValue
    }

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
