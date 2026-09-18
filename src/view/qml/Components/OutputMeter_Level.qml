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
import QtQuick.Layouts 1.15
import Noteahead 1.0

// Level of a device's output, in dBFS: the peak as a number, the RMS as the bar under it.
//
// The unweighted view of the same signal the loudness view measures, and the one that answers the
// two questions LUFS cannot. Peak is headroom -- what the clip LED will latch on if it keeps
// rising. The bar is a 300 ms RMS, which is VU integration time, so it reads as the energy the
// device puts into the mix rather than as its perceived loudness: two devices matched here are
// matched on energy, which is the balance K-weighting throws away at the bottom of the spectrum.
Item {
    id: root

    //! Floor of the measurement. Nothing is arriving while a reading sits here.
    readonly property real minimumDb: -120

    property real peakDb: minimumDb
    property real rmsDb: minimumDb
    //! Where a gain staged device should be reading, drawn as the marker on the bar.
    property real markerDb: -18

    //! Set by OutputMeter, which owns the switching this readout is one of the views of.
    property string switchHint: ""

    readonly property bool hasReading: root.peakDb > root.minimumDb

    implicitWidth: 84
    implicitHeight: column.implicitHeight

    function formatDb(value) {
        return value > root.minimumDb ? value.toFixed(1) : "–";
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 2

        RowLayout {
            spacing: 3
            Layout.fillWidth: true

            Text {
                text: root.formatDb(root.peakDb)
                // Red once the peak is within a dB of full scale, which is the same threshold the
                // bar itself turns at.
                color: root.peakDb > -1 ? "#ff6060" : (root.hasReading ? "white" : "#666")
                font.pointSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }

            Text {
                // Not translated: a unit is a unit in every localised DAW there is.
                text: "dBFS"
                color: "#888"
                font.pointSize: 8
            }
        }

        LevelMeterBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 12
            peakDb: root.peakDb
            rmsDb: root.rmsDb
            markerDb: root.markerDb
        }
    }

    ToolTip.visible: hoverArea.containsMouse
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.text: qsTr("Output level of this device: %1 dBFS peak, %2 dBFS RMS.").arg(root.formatDb(root.peakDb)).arg(root.formatDb(root.rmsDb)) + "\n" + qsTr("The peak is the headroom this device takes; the bar is a 300 ms RMS, the energy it puts into the mix. Neither is weighted, so two devices matched here are matched on energy rather than on perceived loudness.") + "\n" + qsTr("Measured after the insert effects, the fader and the pan. The marker is the gain staging target, %1 dBFS.").arg(root.markerDb) + (root.switchHint === "" ? "" : "\n" + root.switchHint)

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
