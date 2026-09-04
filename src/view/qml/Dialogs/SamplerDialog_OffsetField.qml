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

//! A labeled second-and-millisecond editor for one of the Sampler's pad offsets.
ColumnLayout {
    id: root

    property string label: ""
    property string toolTip: ""
    property int seconds: 0
    property int milliseconds: 0

    signal secondsModified(int value)
    signal millisecondsModified(int value)

    Label {
        text: root.label
        color: "white"
    }

    RowLayout {
        Layout.fillWidth: true
        SpinBox {
            Layout.fillWidth: true
            from: 0
            to: 3600
            value: root.seconds
            editable: true
            onValueModified: root.secondsModified(value)
            Keys.onReturnPressed: {
                value = valueFromText(contentItem.text, locale);
                root.secondsModified(value);
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered && root.toolTip !== ""
            ToolTip.text: root.toolTip
        }
        Label {
            text: "s"
            color: "white"
        }
        SpinBox {
            Layout.fillWidth: true
            from: 0
            to: 999
            value: root.milliseconds
            editable: true
            onValueModified: root.millisecondsModified(value)
            Keys.onReturnPressed: {
                value = valueFromText(contentItem.text, locale);
                root.millisecondsModified(value);
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered && root.toolTip !== ""
            ToolTip.text: root.toolTip
        }
        Label {
            text: "ms"
            color: "white"
        }
    }
}
