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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0

//! The preset row of an effect dialog: the built-in patches and the user's own in one dropdown, and
//! Save and Delete for the user's own.
//!
//! Sits above the rest of the panel rather than inside it, because a preset writes the whole panel.
//! Hides itself when the effect ships no patches and the user has saved none, so an effect that has
//! no presets is not given an empty row.
RowLayout {
    id: root

    //! The slot this row's effect sits in. Set by the dialog, which knows it.
    property int effectIndex: -1

    //! Read once per change rather than bound: every entry is built from the preset store on disk,
    //! so a binding would re-read the directory on every unrelated revision bump.
    property var presetNames: []
    property int currentIndex: 0
    property bool currentIsUserPreset: false

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    spacing: 10
    visible: presetNames.length > 0

    function refresh(): void {
        if (effectIndex < 0) {
            presetNames = [];
            return;
        }
        presetNames = effectRackController.effectPresetNames(effectIndex);
        currentIndex = effectRackController.currentEffectPresetIndex(effectIndex);
        currentIsUserPreset = effectRackController.currentEffectPresetIsUserPreset(effectIndex);
    }

    onEffectIndexChanged: refresh()
    Component.onCompleted: refresh()

    Connections {
        target: effectRackController
        function onEffectPresetsChanged(effectIndex) {
            if (effectIndex === root.effectIndex) {
                root.refresh();
            }
        }
    }

    //! Stands in for a device controller so that the shared Save and Delete dialogs, which know only
    //! how to ask a controller, can serve an effect slot too.
    property QtObject presetTarget: QtObject {
        function userPresetExists(name: string): bool {
            return effectRackController.effectUserPresetExists(root.effectIndex, name);
        }
        function saveUserPreset(name: string): bool {
            return effectRackController.saveEffectUserPreset(root.effectIndex, name);
        }
        function deleteCurrentUserPreset(): bool {
            return effectRackController.deleteCurrentEffectUserPreset(root.effectIndex);
        }
    }

    Label {
        text: qsTr("Preset:")
    }

    ComboBox {
        id: presetCombo
        model: root.presetNames
        currentIndex: root.currentIndex
        onActivated: index => effectRackController.loadEffectPreset(root.effectIndex, index)
        // A ComboBox writes its own currentIndex when the model changes, which breaks the binding
        // above. Without this a preset just saved would not show as the one selected.
        onModelChanged: currentIndex = Qt.binding(() => root.currentIndex)
        Layout.preferredWidth: 240
    }

    AppButton {
        text: qsTr("Save")
        toolTipText: qsTr("Save the current settings as a preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        onClicked: UiService.requestPresetName(root.presetTarget, "")
    }

    AppButton {
        text: qsTr("Delete")
        toolTipText: qsTr("Delete the selected preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        // Only the user's own presets are theirs to delete
        enabled: root.currentIsUserPreset
        onClicked: UiService.requestPresetDeleteConfirmation(root.presetTarget, effectRackController.currentEffectUserPresetName(root.effectIndex))
    }

    Item {
        Layout.fillWidth: true
    }
}
