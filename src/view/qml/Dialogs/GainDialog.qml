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
import "../Components"

EffectDialog {
    id: root
    title: "<strong>" + qsTr("Gain (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.effectDialogScale : 480
    height: parent ? parent.height * Constants.effectDialogScale : 300

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    property bool clipped: false

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ScrollView {
        id: dialogScrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: dialogScrollView.availableWidth
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                Knob {
                    label: qsTr("Gain")
                    suffix: "dB"
                    from: -24
                    to: 24
                    value: {
                        effectRackController.revision;
                        return (effectRackController.parameterValue(root.effectIndex, effectRackController.gainGainKey()) - 0.5) * 48;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gainGainKey(), v / 48 + 0.5)
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    spacing: 4
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        text: qsTr("Clip")
                        font.pixelSize: 11
                        color: themeService.accentColor
                        Layout.alignment: Qt.AlignHCenter
                    }

                    ClipLed {
                        clipped: root.clipped
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            effectRackController.clearGainClip(root.effectIndex);
                            root.clipped = false;
                        }
                    }
                }
            }

            Label {
                text: qsTr("The clip light comes on when the trim pushes the signal to full scale. Click the light to clear it.")
                wrapMode: Text.WordWrap
                color: "#aaaaaa"
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("To make a finished mix louder, reach for the Limiter instead: it catches what a boost pushes past full scale, which a plain trim cannot.")
                wrapMode: Text.WordWrap
                color: themeService.accentColor
                font.pixelSize: 12
                font.italic: true
                Layout.fillWidth: true
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    // The latch itself lives in the effect and survives the dialog being shut, so this only has to
    // keep up with it while it is open. A tenth of a second is far quicker than the eye needs.
    Timer {
        interval: 100
        running: root.opened
        repeat: true
        onTriggered: {
            root.clipped = effectRackController.gainClipped(root.effectIndex);
        }
    }
}
