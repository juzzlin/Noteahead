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

// Reusable stereo oscilloscope: two live traces (L / R) side by side, driven by any
// DeviceController. Capture runs only while "active" is true.
//
// "active" defaults to this item's visibility, but a nested item's visible property is not a
// reliable signal inside a Popup/Dialog (closing the dialog does not always propagate to it).
// Hosts should therefore bind "active" to a dependable condition (e.g. the dialog's own visible
// combined with the current tab) so capture provably stops when the scope is not shown.
Item {
    id: root

    property var deviceController: null
    property int fps: 30
    property color accentColor: themeService.accentColor
    property bool active: visible

    // How many periods of the played note fill the width. The scope asks the device for a window
    // that long, so the trace stands still and shows the same shape whatever note is played,
    // instead of stretching and sliding with the pitch.
    property int cycles: 2

    // Vertical scale follows the signal, so a quiet patch still fills the display. Smoothed, and
    // both channels always share one scale: scaling them apart would misrepresent the stereo image.
    property bool autoZoom: true

    // Fastest the auto scale may move, per frame, as a fraction of the way to the target. Rising
    // quickly enough not to clip a note's attack off the top, falling slowly enough that the trace
    // does not breathe on every decay.
    readonly property real zoomAttack: 0.5
    readonly property real zoomRelease: 0.05

    // Quietest trace the auto scale will still open up for. Below this the display is left alone,
    // so silence between notes does not zoom the noise floor up to full height.
    readonly property real zoomFloor: 0.01

    property real appliedGain: 1.0

    // Pitch the lock is following, refreshed with the trace. Zero when there is none.
    property real lockedFrequency: 0

    onActiveChanged: {
        if (deviceController) {
            deviceController.setScopeActive(active);
        }
    }

    Component.onCompleted: {
        if (deviceController && active) {
            deviceController.setScopeActive(true);
        }
    }

    Component.onDestruction: {
        if (deviceController) {
            deviceController.setScopeActive(false);
        }
    }

    Timer {
        interval: Math.max(16, Math.round(1000 / root.fps))
        running: root.active && root.deviceController !== null
        repeat: true
        onTriggered: {
            const points = Math.max(2, Math.round(leftScope.width));
            const samples = root.deviceController.scopeSamples(points, root.cycles);
            if (samples && samples.length === 2) {
                leftScope.samples = samples[0];
                rightScope.samples = samples[1];
                root.lockedFrequency = root.deviceController.scopeFrequency();
                root.updateZoom();
            }
        }
    }

    // One scale for both channels, eased towards the peak rather than snapped to it.
    function updateZoom(): void {
        if (!autoZoom) {
            appliedGain = 1.0;
            leftScope.gain = 1.0;
            rightScope.gain = 1.0;
            return;
        }
        const peak = Math.max(leftScope.peak, rightScope.peak);
        // 0.9 rather than 1.0: a trace touching the frame reads as clipped even when it is not.
        const target = peak > zoomFloor ? 0.9 / peak : appliedGain;
        const rate = target > appliedGain ? zoomAttack : zoomRelease;
        appliedGain += (target - appliedGain) * rate;
        leftScope.gain = appliedGain;
        rightScope.gain = appliedGain;
    }

    onAutoZoomChanged: updateZoom()

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: qsTr("Cycles")
                color: root.accentColor
                font.pixelSize: 12
            }
            SpinBox {
                id: cycleSpin
                from: 1
                to: 16
                value: root.cycles
                editable: false
                implicitWidth: 110
                onValueModified: root.cycles = value
            }
            Label {
                // The control's own label would come out in the default palette -- black on the
                // dialog's dark ground -- so the text is carried here, coloured like the rest.
                text: qsTr("Auto zoom")
                color: root.accentColor
                font.pixelSize: 12
            }
            Switch {
                checked: root.autoZoom
                onToggled: root.autoZoom = checked
                Universal.theme: Universal.Dark
            }
            Label {
                // What the lock is following, so it is obvious when there is no pitch to lock to
                // and the scope has fallen back to its fixed window.
                text: root.lockedFrequency > 0 ? qsTr("%1 Hz").arg(Math.round(root.lockedFrequency)) : qsTr("free")
                color: "#888"
                font.pixelSize: 11
                font.family: "Monospace"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
        }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            Label {
                text: qsTr("L")
                color: root.accentColor
                font.bold: true
                font.pixelSize: 12
            }
            OscilloscopeRenderer {
                id: leftScope
                Layout.fillWidth: true
                Layout.fillHeight: true
                accentColor: root.accentColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            Label {
                text: qsTr("R")
                color: root.accentColor
                font.bold: true
                font.pixelSize: 12
            }
            OscilloscopeRenderer {
                id: rightScope
                Layout.fillWidth: true
                Layout.fillHeight: true
                accentColor: root.accentColor
            }
        }
    }
    }
}
