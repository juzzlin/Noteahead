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

Rectangle {
    id: rootItem
    color: model.color
    border.color: "#222222"
    border.width: 0
    opacity: !model.isVirtualRow
    Text {
        id: textElement
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.centerIn: parent
        text: model.line
        color: note && note !== "---" ? "#ffffff" : "#888888"
    }
    Rectangle {
        width: {
            if (model.lineColumn === 0) return 3 * (textElement ? textElement.contentWidth : 0) / 14;
            return (textElement ? textElement.contentWidth : 0) / 14;
        }
        height: textElement.contentHeight
        x: {
            if (model.lineColumn === 0) return (textElement ? textElement.x : 0);
            const cw = textElement ? textElement.contentWidth / 14 : 0;
            const tx = textElement ? textElement.x : 0;
            if (model.lineColumn <= 3) return tx + (3 + model.lineColumn) * cw;
            if (model.lineColumn <= 5) return tx + (4 + model.lineColumn) * cw;
            return tx + (5 + model.lineColumn) * cw;
        }
        color: "red"
        opacity: 0.5
        visible: model.isFocused
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            handleClickOnDelegate(index, rootItem, mouse);
        }
        onPressed: mouse => {
            handlePressOnDelegate(index, rootItem, mouse);
        }
        onReleased: mouse => {
            handleReleaseOnDelegate(index, rootItem, mouse);
        }
        onPositionChanged: mouse => {
            handleMouseMoveOnDelegate(index, rootItem, mouse);
        }
    }
}
