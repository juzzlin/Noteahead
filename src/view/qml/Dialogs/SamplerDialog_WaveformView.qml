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
    //! Bumped while a take runs, so the clock below re-reads the elapsed time. The time itself is
    //! polled rather than signalled: it changes continuously and is only shown to a tenth.
    property int recordingTick: 0
    waveformData: currentWaveformData
    fileName: {
        if (samplerController.selectedPad < 0)
            return "";
        const sample = samplerController.padModel.data(samplerController.padModel.index(samplerController.selectedPad, 0), SamplerPadModel.FilePath);
        return sample ? sample.split("/").pop() : "";
    }

    // An empty view says what to do with it. Held back while a take runs, because the clock sits in
    // the same place and says something more useful.
    placeholderText: samplerController.recording ? "" : qsTr("Record audio or assign a file")

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
    audibleLength: samplerController.selectedPadAudibleLength
    envelopeAttack: samplerController.selectedPadAttackSeconds
    envelopeHold: samplerController.selectedPadHoldSeconds
    envelopeDecay: samplerController.selectedPadDecaySeconds
    envelopeSustain: samplerController.selectedPadSustain
    envelopeRelease: samplerController.selectedPadReleaseSeconds
    envelopeCurve: samplerController.selectedPadCurve

    // An empty pad has nothing to trim, so it gets no handles either.
    draggableMarkers: fileName !== ""

    onStartOffsetMoved: newPosition => waveform.writeStartOffset(newPosition * samplerController.selectedPadDuration)
    // The end offset is a trim counted back from the end of the file, not a position in it.
    onEndOffsetMoved: newPosition => waveform.writeEndOffset((1.0 - newPosition) * samplerController.selectedPadDuration)
    // The loop point is counted in from the beginning of the range, which is where the start offset
    // left it.
    onLoopPositionMoved: newPosition => waveform.writeLoopStart((newPosition - waveform.startOffset) * samplerController.selectedPadDuration)

    Timer {
        interval: 20
        running: samplerDialogVisible && waveform.fileName !== ""
        repeat: true
        onTriggered: {
            samplerController.updatePlaybackStatus();
        }
    }

    // How long the take has been running, where the waveform is not yet. A recording you cannot do
    // twice is worth knowing the length of while you are making it.
    Text {
        anchors.centerIn: parent
        visible: samplerController.recording
        color: themeService.accentColor
        // The count-in is read at a glance from across a room with an instrument in your hands.
        font.pixelSize: samplerController.countInBeatsRemaining > 0 ? Constants.waveViewOverlayFontSize * 2 : Constants.waveViewOverlayFontSize
        font.bold: true
        // Rounded to tenths first and split afterwards. Splitting first and rounding the seconds lets
        // the rounding cross a boundary the minutes have already been taken from: 9.96 s read as
        // "0:010.0", and 59.96 s would have read as "0:60.0".
        // Counting down through the pre-count, then counting the take up. The count-in is the more
        // useful of the two while it runs: it says when to come in.
        text: {
            waveform.recordingTick;
            if (samplerController.countInBeatsRemaining > 0) {
                return samplerController.countInBeatsRemaining;
            }
            const tenths = Math.round(samplerController.recordingSeconds() * 10);
            const minutes = Math.floor(tenths / 600);
            const withinMinute = tenths - minutes * 600;
            const seconds = Math.floor(withinMinute / 10);
            return minutes + ":" + (seconds < 10 ? "0" : "") + seconds + "." + (withinMinute % 10);
        }
    }

    Timer {
        interval: 100
        running: waveform.samplerDialogVisible && samplerController.recording
        repeat: true
        onTriggered: waveform.recordingTick++
    }

    //! The offsets are two properties, a whole second and a millisecond, and rounding to whole
    //! milliseconds before splitting keeps the two halves from disagreeing at the seam.
    function splitSeconds(seconds) {
        const milliseconds = Math.max(0, Math.round(seconds * 1000));
        return [Math.floor(milliseconds / 1000), milliseconds % 1000];
    }

    function writeStartOffset(seconds) {
        const parts = splitSeconds(seconds);
        samplerController.selectedPadStartOffsetSeconds = parts[0];
        samplerController.selectedPadStartOffsetMilliseconds = parts[1];
    }

    function writeEndOffset(seconds) {
        const parts = splitSeconds(seconds);
        samplerController.selectedPadEndOffsetSeconds = parts[0];
        samplerController.selectedPadEndOffsetMilliseconds = parts[1];
    }

    function writeLoopStart(seconds) {
        const parts = splitSeconds(seconds);
        samplerController.selectedPadLoopStartSeconds = parts[0];
        samplerController.selectedPadLoopStartMilliseconds = parts[1];
    }

    //! What the picture is drawn from. Everything else about a pad -- its trims, its envelope, the
    //! loop point -- is drawn on top of the picture rather than into it.
    property string waveformSource: ""

    function updateWaveform() {
        if (width <= 0) {
            return;
        }
        // Rebuilding the picture means reading the file again, and the pad emits its dataChanged for
        // every knob and every dragged marker. Only the things the picture is made of are a reason to.
        const source = [samplerController.selectedPad, waveform.fileName, samplerController.selectedPadDuration, samplerController.selectedPadReverse, Math.round(width)].join("/");
        if (source === waveformSource) {
            return;
        }
        const data = samplerController.getWaveformData(width - 12);
        currentWaveformData = data || [];
        // A picture that could not be read is not one to remember: the next attempt has to try again.
        waveformSource = currentWaveformData.length ? source : "";
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
