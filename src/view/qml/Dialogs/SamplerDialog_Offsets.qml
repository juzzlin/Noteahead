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

ColumnLayout {
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 15

    SamplerDialog_OffsetField {
        Layout.fillWidth: true
        label: qsTr("Start Offset:")
        toolTip: qsTr("Seconds skipped at the beginning of the sample.")
        seconds: samplerController.selectedPadStartOffsetSeconds
        milliseconds: samplerController.selectedPadStartOffsetMilliseconds
        onSecondsModified: value => samplerController.selectedPadStartOffsetSeconds = value
        onMillisecondsModified: value => samplerController.selectedPadStartOffsetMilliseconds = value
    }

    SamplerDialog_OffsetField {
        Layout.fillWidth: true
        label: qsTr("End Offset:")
        toolTip: qsTr("Seconds trimmed off the end of the sample.")
        seconds: samplerController.selectedPadEndOffsetSeconds
        milliseconds: samplerController.selectedPadEndOffsetMilliseconds
        onSecondsModified: value => samplerController.selectedPadEndOffsetSeconds = value
        onMillisecondsModified: value => samplerController.selectedPadEndOffsetMilliseconds = value
    }
}
