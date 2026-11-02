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
import Noteahead 1.0
import ".."

// A section title with a waveform drawn after it on the same line, as AutomationRowHeading does
// with a curve. The preview is only a little taller than the title, so the row barely grows.
Item {
    id: root

    property string text: ""
    //! The waveform from left to right, -1 at the bottom and 1 at the top.
    property var samples: []
    //! For a source that is shaped but not heard.
    property bool dimmed: false
    //! As many points as the preview is pixels wide, for whoever supplies the samples.
    readonly property int previewPoints: Math.max(2, Math.round(preview.width))

    // A width as well as a height, or a layout or GroupBox holding this treats it as empty
    implicitWidth: heading.implicitWidth
    implicitHeight: heading.implicitHeight + 2

    SectionHeading {
        id: heading
        text: root.text
        anchors.verticalCenter: parent.verticalCenter
        // The title first: the preview takes what is left of the line
        width: Math.min(implicitWidth, root.width)
        elide: Text.ElideRight
    }

    WaveShapeRenderer {
        id: preview
        anchors.left: heading.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: width > 20
        samples: root.samples
        dimmed: root.dimmed
        accentColor: themeService.accentColor
    }
}
