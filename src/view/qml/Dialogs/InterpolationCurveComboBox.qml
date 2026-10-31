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
import QtQuick.Controls.Universal 2.15
import ".."

// The one place the curve names live. The order must match Interpolator::CurveType, because the
// index is what gets handed to the automation service and stored in the project.
ComboBox {
    //! Drawn is offered only where there is an automation to draw: the add forms have none until
    //! they are accepted.
    property bool drawnAvailable: false
    //! Interpolator::CurveType::Drawn
    readonly property int drawnIndex: 6
    model: drawnAvailable ? [qsTr("Linear"), qsTr("Exponential"), qsTr("Logarithmic"), qsTr("Ease In"), qsTr("Ease Out"), qsTr("Ease In/Out"), qsTr("Drawn")] : [qsTr("Linear"), qsTr("Exponential"), qsTr("Logarithmic"), qsTr("Ease In"), qsTr("Ease Out"), qsTr("Ease In/Out")]
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("The shape of the ramp between the start and end value")
}
