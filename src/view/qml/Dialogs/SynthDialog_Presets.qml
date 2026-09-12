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
import "../Components"

//! Sits above the tabs rather than inside one: a preset writes the whole panel, so putting it under
//! any single tab would say otherwise.
RowLayout {
    id: root

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    spacing: 10

    Label {
        text: qsTr("Preset:")
    }

    ComboBox {
        id: presetCombo
        model: synthController.presetNames
        currentIndex: synthController.currentPresetIndex
        onActivated: index => synthController.loadPreset(index)
        // A ComboBox writes its own currentIndex when the model changes, which breaks the binding
        // above. Without this a preset just saved would not show as the one selected.
        onModelChanged: currentIndex = Qt.binding(() => synthController.currentPresetIndex)
        Layout.preferredWidth: 220
    }

    AppButton {
        text: qsTr("Save preset...")
        toolTipText: qsTr("Save the current settings as a preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        onClicked: UiService.requestPresetName(synthController, "")
    }

    AppButton {
        text: qsTr("Delete")
        toolTipText: qsTr("Delete the selected preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        // Only the user's own presets are theirs to delete
        enabled: synthController.currentPresetIsUserPreset
        onClicked: UiService.requestPresetDeleteConfirmation(synthController, synthController.currentUserPresetName())
    }

    Label {
        text: qsTr("A general purpose 6-voice synthesizer")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
    }
}
