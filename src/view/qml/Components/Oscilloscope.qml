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

// Reusable stereo oscilloscope: two live traces (L / R) side by side, driven by any
// DeviceController. Capture runs only while this item is visible.
Item {
    id: root

    property var deviceController: null
    property int fps: 30
    property color accentColor: themeService.accentColor

    onVisibleChanged: {
        if (deviceController) {
            deviceController.setScopeActive(visible);
        }
    }

    Component.onDestruction: {
        if (deviceController) {
            deviceController.setScopeActive(false);
        }
    }

    Timer {
        interval: Math.max(16, Math.round(1000 / root.fps))
        running: root.visible && root.deviceController !== null
        repeat: true
        onTriggered: {
            const points = Math.max(2, Math.round(leftScope.width));
            const samples = root.deviceController.scopeSamples(points);
            if (samples && samples.length === 2) {
                leftScope.samples = samples[0];
                rightScope.samples = samples[1];
            }
        }
    }

    RowLayout {
        anchors.fill: parent
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
