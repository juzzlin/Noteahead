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
import QtQuick.Controls.Universal
import ".."

// A ComboBox's popup that lays a long list out in columns, read top to bottom, instead of one tall
// scrolling column. A short list opens below the box as usual.
//   ComboBox { id: box; popup: ColumnsPopup { comboBox: box } }
Popup {
    id: root
    required property ComboBox comboBox
    //! Columns for a long list
    property int columns: 4
    //! The longest list still shown as one column
    property int maxSingleColumnCount: 24
    //! The tallest a single column gets before it scrolls
    property int maxSingleColumnHeight: 300
    readonly property int count: comboBox.count
    readonly property bool inColumns: count > maxSingleColumnCount
    readonly property int edgeMargin: 10
    readonly property int cellHeight: inColumns ? 22 : 32
    readonly property int rows: inColumns ? Math.ceil(count / columns) : count
    readonly property real roomWidth: (parent ? parent.width : comboBox.width) - 2 * edgeMargin - 2 * padding
    readonly property real roomHeight: (parent ? parent.height : maxSingleColumnHeight) - 2 * edgeMargin - 2 * padding
    readonly property real longestTextWidth: {
        let widest = 0;
        for (let index = 0; index < count; index++) {
            widest = Math.max(widest, textMetrics.advanceWidth(comboBox.textAt(index)));
        }
        return widest;
    }
    readonly property real cellWidth: inColumns ? Math.min(longestTextWidth + 2 * cellPadding + 4, roomWidth / columns) : comboBox.width - 2 * padding
    readonly property int cellPadding: 8
    parent: Overlay.overlay
    padding: 1
    width: (inColumns ? columns * cellWidth : cellWidth) + 2 * padding
    height: Math.min(rows * cellHeight, inColumns ? roomHeight : maxSingleColumnHeight) + 2 * padding
    // Placed when it opens: centred over the window in columns, otherwise below the box
    onAboutToShow: {
        if (inColumns) {
            x = Math.max(edgeMargin, (parent.width - width) / 2);
            y = Math.max(edgeMargin, (parent.height - height) / 2);
        } else {
            const below = comboBox.mapToItem(parent, 0, comboBox.height - 1);
            x = below.x;
            y = Math.min(below.y, parent.height - height - edgeMargin);
        }
        grid.positionViewAtIndex(Math.max(0, comboBox.currentIndex), GridView.Contain);
    }
    FontMetrics {
        id: textMetrics
        font: root.comboBox.font
    }
    contentItem: GridView {
        id: grid
        clip: true
        // A column fills before the next one starts, so the list still reads in order
        flow: root.inColumns ? GridView.FlowTopToBottom : GridView.FlowLeftToRight
        cellWidth: root.cellWidth
        cellHeight: root.cellHeight
        model: root.visible ? root.count : 0
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            policy: root.inColumns ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded
        }
        ScrollBar.horizontal: ScrollBar {
            policy: root.inColumns ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
        delegate: ItemDelegate {
            id: cell
            required property int index
            width: grid.cellWidth
            height: grid.cellHeight
            topPadding: 0
            bottomPadding: 0
            leftPadding: root.cellPadding
            rightPadding: root.cellPadding
            text: root.comboBox.textAt(index)
            font: root.comboBox.font
            highlighted: root.comboBox.highlightedIndex === index || hovered
            Universal.theme: Universal.Dark
            contentItem: Text {
                id: label
                text: cell.text
                font: cell.font
                color: cell.index === root.comboBox.currentIndex && !cell.highlighted ? Universal.accent : Universal.foreground
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            ToolTip.visible: hovered && label.truncated
            ToolTip.text: text
            ToolTip.delay: Constants.toolTipDelay
            onClicked: {
                root.comboBox.currentIndex = index;
                root.comboBox.activated(index);
                root.close();
            }
        }
    }
    background: Rectangle {
        color: "#303030"
        border.color: "#606060"
    }
}
