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
    Label {
        text: qsTr("Start Offset:")
        color: "white"
    }
    RowLayout {
        Layout.fillWidth: true
        SpinBox {
            id: secondsSpinBox
            Layout.fillWidth: true
            from: 0
            to: 3600
            value: samplerController.selectedPadStartOffsetSeconds
            editable: true
            onValueModified: samplerController.selectedPadStartOffsetSeconds = value
            Keys.onReturnPressed: {
                value = valueFromText(contentItem.text, locale);
                samplerController.selectedPadStartOffsetSeconds = value;
            }
        }
        Label {
            text: "s"
            color: "white"
        }
        SpinBox {
            id: msSpinBox
            Layout.fillWidth: true
            from: 0
            to: 999
            value: samplerController.selectedPadStartOffsetMilliseconds
            editable: true
            onValueModified: samplerController.selectedPadStartOffsetMilliseconds = value
            Keys.onReturnPressed: {
                value = valueFromText(contentItem.text, locale);
                samplerController.selectedPadStartOffsetMilliseconds = value;
            }
        }
        Label {
            text: "ms"
            color: "white"
        }
    }
}
