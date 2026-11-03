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
import QtQuick.Layouts 1.15
import Noteahead 1.0

//! One hit of a single drum such as Kick 808 or Kick 909, drawn as a drum machine draws a voice:
//! the drum untouched, the amp envelope laid over it and the playhead running while it sounds.
//! Rendered rather than read from a file, so the model decides when to redraw.
WaveformView {
    id: root

    //! The drum's DrumHitWaveformModel, its dialog model's `waveform`.
    required property var model
    //! The dialog holding this, so nothing is rendered or polled while it is shut.
    property bool dialogVisible: false

    Layout.fillWidth: true
    Layout.preferredHeight: 150

    waveformData: root.model.waveformData
    duration: root.model.waveformDuration
    audibleLength: root.model.audibleLength

    // Only while the drum sounds: a bar parked at the left between hits would say it is playing
    showPlayhead: root.model.sounding
    playbackPosition: root.model.playbackPosition

    showEnvelope: true
    envelopeAttack: root.model.envelopeAttack
    envelopeHold: root.model.envelopeHold
    envelopeDecay: root.model.envelopeDecay
    envelopeSustain: root.model.envelopeSustain
    envelopeRelease: root.model.envelopeRelease
    envelopeCurve: root.model.envelopeCurve

    // The picture is as wide as the view, so a resize invalidates it as surely as a knob does
    function requestWaveform(): void {
        root.model.setRequest(Math.max(0, Math.floor(width) - 12), root.dialogVisible);
    }

    // As the drum machines' waveform polls, and only while the dialog is open: the playhead moves
    // with the audio, which nothing in the device signals about
    Timer {
        interval: 20
        running: root.dialogVisible
        repeat: true
        onTriggered: root.model.updatePlaybackStatus()
    }

    onWidthChanged: requestWaveform()
    onDialogVisibleChanged: requestWaveform()
    Component.onCompleted: requestWaveform()
}
