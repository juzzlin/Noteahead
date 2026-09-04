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
import QtQuick.Layouts
import Noteahead 1.0
import "../Components"

WaveformView {
    id: waveform
    Layout.fillWidth: true
    Layout.preferredHeight: 150
    Layout.margins: 10

    property bool samplerDialogVisible: false
    property var currentWaveformData: []
    waveformData: currentWaveformData
    fileName: {
        if (samplerController.selectedPad < 0)
            return "";
        const sample = samplerController.padModel.data(samplerController.padModel.index(samplerController.selectedPad, 0), SamplerPadModel.FilePath);
        return sample ? sample.split("/").pop() : "";
    }

    playbackPosition: samplerController.playbackPosition
    startOffset: {
        if (samplerController.selectedPadDuration > 0) {
            return (samplerController.selectedPadStartOffsetSeconds + samplerController.selectedPadStartOffsetMilliseconds / 1000.0) / samplerController.selectedPadDuration;
        }
        return 0.0;
    }
    endOffset: {
        if (samplerController.selectedPadDuration > 0) {
            const trim = samplerController.selectedPadEndOffsetSeconds + samplerController.selectedPadEndOffsetMilliseconds / 1000.0;
            return 1.0 - trim / samplerController.selectedPadDuration;
        }
        return 1.0;
    }
    showPlayhead: fileName !== ""

    loopPosition: {
        if (!samplerController.selectedPadLoop || samplerController.selectedPadDuration <= 0) {
            return -1.0;
        }
        const loopStart = samplerController.selectedPadLoopStartSeconds + samplerController.selectedPadLoopStartMilliseconds / 1000.0;
        return Math.min(endOffset, startOffset + loopStart / samplerController.selectedPadDuration);
    }

    showEnvelope: fileName !== ""
    duration: samplerController.selectedPadDuration
    envelopeAttack: samplerController.selectedPadAttackSeconds
    envelopeDecay: samplerController.selectedPadDecaySeconds
    envelopeSustain: samplerController.selectedPadSustain
    envelopeRelease: samplerController.selectedPadReleaseSeconds

    Timer {
        interval: 20
        running: samplerDialogVisible && waveform.fileName !== ""
        repeat: true
        onTriggered: {
            samplerController.updatePlaybackStatus();
        }
    }

    function updateWaveform() {
        if (width > 0) {
            const data = samplerController.getWaveformData(width - 12);
            currentWaveformData = data || [];
        }
    }

    onWidthChanged: updateWaveform()

    Connections {
        target: samplerController
        function onSelectedPadChanged() {
            waveform.updateWaveform();
        }
        // Reversing a pad flips the waveform it is drawn from, so the picture has to be rebuilt.
        function onSelectedPadReverseChanged() {
            waveform.updateWaveform();
        }
    }

    Connections {
        target: samplerController.padModel
        function onDataChanged() {
            waveform.updateWaveform();
        }
    }
}
