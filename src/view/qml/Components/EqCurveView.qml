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

// The equalizer curve as every EQ dialog shows it: the renderer itself, plus the spectrum's dB range,
// which belongs to the view rather than to any one effect. Quick buttons in the top right corner
// while a spectrum is shown, and the same choice on a right click.
EqCurveRenderer {
    id: root

    readonly property var spectrumRanges: [30, 45, 60, 90]
    // Only the spectrum itself is drawn on this scale; a difference is drawn on the curve's
    readonly property bool spectrumShown: spectrumMode >= 1 && spectrumMode <= 3 // Pre, Post or Both

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        spacing: 2
        visible: root.spectrumShown

        Repeater {
            model: root.spectrumRanges
            delegate: Rectangle {
                id: rangeButton
                readonly property bool selected: root.spectrumRange === modelData
                width: rangeLabel.implicitWidth + 8
                height: rangeLabel.implicitHeight + 2
                radius: 2
                color: selected ? themeService.accentColor : (rangeHover.hovered ? Theme.surfaceRaised : Theme.surfacePanel)
                opacity: selected || rangeHover.hovered ? 1.0 : 0.8
                Text {
                    id: rangeLabel
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 9
                    font.family: "monospace"
                    color: rangeButton.selected ? themeService.accentTextColor : Theme.textBright
                }
                HoverHandler {
                    id: rangeHover
                }
                TapHandler {
                    onTapped: root.spectrumRange = modelData
                }
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: rangeHover.hovered
                ToolTip.text: qsTr("How far below full scale the spectrum reaches, in dB")
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        // Nothing to choose while no spectrum is drawn, and an EQ without one never offers it
        enabled: root.spectrumShown
        onTapped: eventPoint => rangeMenu.popup(eventPoint.position.x, eventPoint.position.y)
    }

    Menu {
        id: rangeMenu
        delegate: MenuItemDelegate {}
        Repeater {
            model: root.spectrumRanges
            delegate: MenuItemDelegate {
                // A mark rather than a check box: the delegate draws text only
                text: (root.spectrumRange === modelData ? "● " : "    ") + qsTr("Spectrum Range") + " " + modelData + " dB"
                onClicked: root.spectrumRange = modelData
            }
        }
    }
}
