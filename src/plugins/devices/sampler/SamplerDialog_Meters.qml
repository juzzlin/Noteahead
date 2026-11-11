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
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

// The pair of meters beside the wave view: the record input while recording, and the Sampler's own
// output the rest of the time. Setting a recording level used to mean recording something and looking
// at the waveform afterwards.
RowLayout {
    id: root

    //! Both taps are switched off while the dialog is closed, so the audio thread does nothing for
    //! meters nobody is looking at.
    property bool samplerDialogVisible: false

    spacing: 4

    // The levels of both channels from one call, so the two bars are always the same moment. Polled
    // rather than signalled: the meters are read off atomics the audio thread writes, and 20 a second
    // is what the mixer does.
    property int meterTick: 0
    readonly property var levels: {
        root.meterTick;
        return samplerController.meterLevels();
    }

    function levelOf(key) {
        return root.levels && root.levels[key] !== undefined ? root.levels[key] : -120;
    }

    onSamplerDialogVisibleChanged: samplerController.setMetersActive(samplerDialogVisible)

    Timer {
        interval: 50
        running: root.samplerDialogVisible
        repeat: true
        onTriggered: root.meterTick++
    }

    VerticalLevelMeterBar {
        label: "L"
        Layout.fillHeight: true
        peakDb: root.levelOf("leftPeakDb")
        rmsDb: root.levelOf("leftRmsDb")
        markerDb: settingsService.gainStagingTargetDb
    }

    VerticalLevelMeterBar {
        label: "R"
        Layout.fillHeight: true
        peakDb: root.levelOf("rightPeakDb")
        rmsDb: root.levelOf("rightRmsDb")
        markerDb: settingsService.gainStagingTargetDb
    }
}
