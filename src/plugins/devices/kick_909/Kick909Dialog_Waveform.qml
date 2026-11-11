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

//! One hit of the drum, drawn as Drum Synth V2 draws a voice: the drum untouched, with the amp
//! envelope laid over it. Rendered rather than read from a file, so the controller decides when to
//! redraw and pushes the result here.
WaveformView {
    id: root

    //! The dialog holding this, so nothing is rendered while it is shut.
    property bool dialogVisible: false

    Layout.fillWidth: true
    Layout.preferredHeight: 150

    waveformData: kick909Controller.waveformData
    duration: kick909Controller.waveformDuration
    audibleLength: kick909Controller.audibleLength

    // The envelope the picture was rendered with, so the overlay waits for a drag to settle along
    // with the waveform under it
    showEnvelope: true
    envelopeAttack: kick909Controller.waveformEnvelopeAttack
    envelopeHold: kick909Controller.waveformEnvelopeHold
    envelopeDecay: kick909Controller.waveformEnvelopeDecay
    envelopeSustain: kick909Controller.waveformEnvelopeSustain
    envelopeRelease: kick909Controller.waveformEnvelopeRelease
    envelopeCurve: kick909Controller.waveformEnvelopeCurve

    // The picture is as wide as the view, so a resize invalidates it as surely as a knob does
    function requestWaveform(): void {
        kick909Controller.setWaveformRequest(Math.max(0, Math.floor(width) - 12), root.dialogVisible);
    }

    onWidthChanged: requestWaveform()
    onDialogVisibleChanged: requestWaveform()
    Component.onCompleted: requestWaveform()
}
