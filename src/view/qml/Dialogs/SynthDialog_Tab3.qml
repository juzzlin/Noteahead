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
import Noteahead 1.0

ScrollView {
    clip: true
    property real moduleWidth: 0

    GridLayout {
        columns: 3
        columnSpacing: 20
        width: parent.width - 20
        SynthDialog_Lfo {
            Layout.preferredWidth: moduleWidth
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
        }
        SynthDialog_Delay {
            Layout.preferredWidth: moduleWidth * 2 + 20
            Layout.fillWidth: true
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignTop
        }
    }
}
