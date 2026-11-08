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

//! One note of the bass: the open E plucked at full strength, held for two seconds and let go.
//! Rendered rather than read from a file, so the controller decides when to redraw and pushes the
//! result here.
WaveformView {
    id: root

    //! The dialog holding this, so nothing is rendered while it is shut.
    property bool dialogVisible: false

    Layout.fillWidth: true
    Layout.preferredHeight: 110

    waveformData: electricBassController.waveformData
    duration: electricBassController.waveformDuration

    // The picture is as wide as the view, so a resize invalidates it as surely as a knob does
    function requestWaveform(): void {
        electricBassController.setWaveformRequest(Math.max(0, Math.floor(width) - 12), root.dialogVisible);
    }

    onWidthChanged: requestWaveform()
    onDialogVisibleChanged: requestWaveform()
    Component.onCompleted: requestWaveform()
}
