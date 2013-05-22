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
import Noteahead 1.0
import ".."

// What of the live spectrum an equalizer's curve is drawn over, and what keeps it running.
//
// Nothing runs unless a mode is picked and the dialog is showing: the taps are switched on with the
// mode and off with the dialog, and the timer that fetches the levels runs only while both hold.
// The mode belongs to the dialog rather than to the project, and is not saved.
ComboBox {
    id: root
    //! The slot whose spectrum is shown.
    property int effectIndex: -1
    //! The EqCurveRenderer the spectrum is drawn in.
    property var renderer: null
    //! Whether the dialog is showing.
    property bool active: false
    // The slot whose taps were last switched on, so that handing the dialog another slot switches
    // the old one off rather than leaving it running unseen.
    property int activeEffectIndex: -1

    //! The EqCurveRenderer.SpectrumMode each entry draws, the first being off. An equalizer whose
    //! taps mean something else names them in its own model, as Dynamic EQ does its side-chain.
    property var modes: [0, 1, 2, 3, 4, 5]

    implicitWidth: 130
    // In the order of EqCurveRenderer.SpectrumMode
    model: [qsTr("Off"), "Pre", "Post", qsTr("Both"), "Pre − Post", "Post − Pre"]

    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("The live spectrum behind the curve: of the input, of the output, or both. A difference is drawn on the curve's scale, so where it leaves the curve, something the curve does not show is at work.")

    function updateTaps(): void {
        if (!renderer) {
            return;
        }
        if (activeEffectIndex >= 0 && activeEffectIndex !== effectIndex) {
            effectRackController.spectrumSetActive(activeEffectIndex, renderer, false);
        }
        // Off for an index with no mode, as while the combo is still being built: the renderer is
        // handed over before the model is, which leaves currentIndex at -1
        const mode = modes[currentIndex] ?? 0;
        renderer.spectrumMode = mode;
        const running = active && effectIndex >= 0 && mode > 0; // 0 is Off
        effectRackController.spectrumSetActive(effectIndex, renderer, running);
        activeEffectIndex = running ? effectIndex : -1;
    }

    onActivated: updateTaps()
    onActiveChanged: updateTaps()
    onEffectIndexChanged: updateTaps()
    onRendererChanged: updateTaps()

    Timer {
        // Thirty frames a second: the analysis behind it updates no faster than that at the low end
        interval: 33
        running: root.activeEffectIndex >= 0
        repeat: true
        onTriggered: effectRackController.spectrumUpdateRenderer(root.effectIndex, root.renderer)
    }
}
