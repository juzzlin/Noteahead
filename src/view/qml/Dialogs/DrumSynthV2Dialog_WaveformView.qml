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
import "../Components"

// The selected voice, drawn the way the Sampler draws a pad: the material untouched, with the amp
// envelope laid over it. The picture is rendered rather than read from a file, so the controller
// decides when to redraw and pushes the result here.
WaveformView {
    id: root

    //! The dialog holding this, so nothing is rendered while it is shut.
    property bool dialogVisible: false

    Layout.fillWidth: true
    Layout.preferredHeight: 150

    waveformData: drumSynthV2Controller.waveformData
    duration: drumSynthV2Controller.waveformDuration
    audibleLength: drumSynthV2Controller.audibleLength

    // The playhead only while something is sounding: a drum is struck and gone, so a bar parked at
    // the left between hits would be saying a voice is playing when none is.
    showPlayhead: drumSynthV2Controller.voiceSounding
    playbackPosition: drumSynthV2Controller.playbackPosition

    showEnvelope: true
    envelopeAttack: drumSynthV2Controller.voiceAmpAttackSeconds
    envelopeHold: drumSynthV2Controller.voiceAmpHoldSeconds
    envelopeDecay: drumSynthV2Controller.voiceAmpDecaySeconds
    envelopeSustain: drumSynthV2Controller.voiceAmpSustain / Constants.uiInternalScaling
    envelopeRelease: drumSynthV2Controller.voiceAmpReleaseSeconds
    envelopeCurve: drumSynthV2Controller.voiceAmpCurve / Constants.uiInternalScaling

    // The picture is as wide as the view, so a resize invalidates it as surely as a knob does. Both
    // go through the controller's own wait, which is what stops a drag on the dialog edge rendering
    // on every frame of it.
    function requestWaveform(): void {
        drumSynthV2Controller.setWaveformRequest(Math.max(0, Math.floor(width) - 12), root.dialogVisible);
    }

    // Twenty milliseconds, as the Sampler's own waveform polls: the playhead moves with the audio,
    // and nothing in the device has reason to signal about that on its own.
    Timer {
        interval: 20
        running: root.dialogVisible
        repeat: true
        onTriggered: drumSynthV2Controller.updatePlaybackStatus()
    }

    onWidthChanged: requestWaveform()
    onDialogVisibleChanged: requestWaveform()
    Component.onCompleted: requestWaveform()
}
