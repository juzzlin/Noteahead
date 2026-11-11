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
        model: juSynth08Controller.presetNames
        currentIndex: juSynth08Controller.currentPresetIndex
        onActivated: index => juSynth08Controller.loadPreset(index)
        onModelChanged: currentIndex = Qt.binding(() => juSynth08Controller.currentPresetIndex)
        Layout.preferredWidth: 220
    }

    AppButton {
        text: qsTr("Save")
        toolTipText: qsTr("Save the current settings as a preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        onClicked: UiService.requestPresetName(juSynth08Controller, "")
    }

    AppButton {
        text: qsTr("Delete")
        toolTipText: qsTr("Delete the selected preset of your own")
        implicitWidth: Constants.defaultButtonWidth
        enabled: juSynth08Controller.currentPresetIsUserPreset
        onClicked: UiService.requestDeleteConfirmation(juSynth08Controller, juSynth08Controller.currentPresetIndex)
    }

    SectionHeading {
        text: qsTr("An eight-voice polysynth after the Jupiter-8")
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
    }
}
