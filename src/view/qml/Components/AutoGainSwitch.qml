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

// The Auto Gain switch every equalizer shares, with what it currently applies beside the name.
Switch {
    id: root
    //! The slot whose switch this is.
    property int effectIndex: -1
    //! What auto gain applies, in dB. Read off the parameters, so it is up to date before
    //! anything has played.
    readonly property real gainDb: {
        effectRackController.revision;
        return checked ? effectRackController.eqAutoGainDb(root.effectIndex) : 0;
    }
    function formatDb(db) {
        const rounded = Math.round(db * 10) / 10;
        return (rounded > 0 ? "+" : rounded < 0 ? "−" : "") + Math.abs(rounded).toFixed(1) + " dB";
    }
    text: qsTr("Auto Gain") + (checked ? " " + formatDb(gainDb) : "")
    checked: {
        effectRackController.revision;
        return effectRackController.parameterValue(root.effectIndex, effectRackController.eqAutoGainKey()) >= 0.5;
    }
    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.eqAutoGainKey(), checked ? 1 : 0)
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Turn the output up or down by what the curve adds or takes away on average, so that comparing with the equalizer bypassed compares the tone, not the level")
}
