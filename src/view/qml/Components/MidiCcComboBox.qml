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
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."
import "../.."

ComboBox {
    id: root
    signal controllerChanged(int controller)
    property string portName: ""
    model: propertyService.getAvailableMidiControllers(portName)
    textRole: "name"
    valueRole: "number"
    editable: true
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Controller. Current selection: ") + root.currentText
    delegate: ItemDelegate {
        width: root.width
        text: modelData.name
        highlighted: root.highlightedIndex === index
        Universal.theme: Universal.Dark
    }
    popup: Popup {
        y: root.height - 1
        width: root.width
        implicitHeight: contentItem.implicitHeight > 300 ? 300 : contentItem.implicitHeight
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }
        }
        background: Rectangle {
            color: "#303030"
            border.color: "#606060"
        }
    }
    function setController(value: int): void {
        currentIndex = indexOfValue(value);
    }
    onActivated: controllerChanged(currentValue)
}
