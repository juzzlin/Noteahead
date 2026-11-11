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

// Two buttons, as on the Juno: either one alone is Chorus I or II, and both down is I + II.
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    readonly property int chorusMode: juSynthController.chorusMode

    SectionHeading {
        text: qsTr("Chorus")
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        AppButton {
            text: "I"
            // Not checkable: AppButton's overlay clicks without toggling, so the bit is flipped here
            checked: (chorusMode & 1) !== 0
            onClicked: juSynthController.chorusMode = chorusMode ^ 1
            Layout.fillWidth: true
        }
        AppButton {
            text: "II"
            checked: (chorusMode & 2) !== 0
            onClicked: juSynthController.chorusMode = chorusMode ^ 2
            Layout.fillWidth: true
        }
    }
}
