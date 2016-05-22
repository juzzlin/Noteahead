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

//! A preset dropdown with Save and Delete for the user's own presets, for whatever keeps presets:
//! an effect slot through EffectPresetRow, or the theme.
//!
//! What it acts on is presetTarget, which the shared Save and Delete dialogs ask too. It has to
//! answer userPresetExists(name), saveUserPreset(name), deleteCurrentUserPreset() and
//! currentUserPresetName().
RowLayout {
    id: root

    property var presetNames: []
    property int currentIndex: 0
    property bool currentIsUserPreset: false
    property var presetTarget: null

    //! The user picked entry @p index.
    signal presetActivated(int index)

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    spacing: 10

    Label {
        text: qsTr("Preset:")
    }

    ComboBox {
        id: presetCombo
        model: root.presetNames
        currentIndex: root.currentIndex
        onActivated: index => root.presetActivated(index)
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
        onClicked: UiService.requestPresetDeleteConfirmation(root.presetTarget, root.presetTarget.currentUserPresetName())
    }

    Item {
        Layout.fillWidth: true
    }
}
