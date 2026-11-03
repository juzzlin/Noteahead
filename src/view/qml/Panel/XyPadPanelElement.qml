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
import Noteahead 1.0

//! The body of an XY pad device's panel element: its four pads in a row, square and as tall as
//! the element, each with what its axes move written along their edges.
Item {
    id: root

    required property string sourceId

    readonly property var controller: performancePanelController.revision >= 0 ? performancePanelController.xyPadController(root.sourceId) : null

    //! The name of the parameter a pad's axis moves, or empty for none.
    function targetLabel(owner: string, ownerId: string, parameter: string): string {
        return ownerId === "" ? "" : (propertyService.automationParameter(owner, ownerId, parameter).label || "");
    }

    implicitWidth: row.implicitWidth

    Row {
        id: row
        height: root.height
        spacing: 6
        Repeater {
            model: root.controller ? root.controller.pads : []
            delegate: XyPadRenderer {
                required property var modelData
                width: row.height
                height: row.height
                controller: root.controller
                pad: modelData.index
                xLabel: root.targetLabel(modelData.xTargetOwner, modelData.xTargetOwnerId, modelData.xTargetParameter)
                yLabel: root.targetLabel(modelData.yTargetOwner, modelData.yTargetOwnerId, modelData.yTargetParameter)
                accentColor: themeService.accentColor
            }
        }
    }
}
