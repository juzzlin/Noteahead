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
    title: "<strong>" + qsTr("Monitor (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.effectDialogScale : 520
    height: parent ? parent.height * Constants.effectDialogScale : 300

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    // The selected mode, read back from the effect rather than held here: the buttons below are not
    // checkable and own no state, so this is the only thing that says which one is lit. Cancel and
    // undo therefore move the buttons without either of them knowing the buttons exist.
    readonly property int mode: {
        effectRackController.revision;
        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.monitorModeKey()));
    }

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [
                    {
                        "label": qsTr("Stereo"),
                        "mode": 0
                    },
                    {
                        "label": qsTr("Mono"),
                        "mode": 1
                    },
                    {
                        "label": qsTr("Left"),
                        "mode": 2
                    },
                    {
                        "label": qsTr("Right"),
                        "mode": 3
                    },
                    {
                        "label": qsTr("Side"),
                        "mode": 4
                    }
                ]
                // Deliberately not checkable and not in a ButtonGroup: AppButton's overlaid MouseArea
                // eats the press that AbstractButton would have toggled itself on, and a monitor is
                // switched while the engine is running, which is exactly the case AppButton exists for.
                // Binding `highlighted` instead keeps the lit state a read of the parameter.
                delegate: AppButton {
                    required property var modelData
                    text: modelData.label
                    highlighted: root.mode === modelData.mode
                    Layout.fillWidth: true
                    onClicked: effectRackController.setParameterValue(root.effectIndex, effectRackController.monitorModeKey(), modelData.mode)
                }
            }
        }

        Label {
            text: {
                switch (root.mode) {
                case 1:
                    return qsTr("Both channels summed at half: what a mono system hears. Parts that lean on stereo width thin out or disappear here.");
                case 2:
                    return qsTr("The left channel on both sides.");
                case 3:
                    return qsTr("The right channel on both sides.");
                case 4:
                    return qsTr("The difference between the channels: exactly the part that mono summing throws away. Near-silence means nothing is lost.");
                default:
                    return qsTr("The mix as it is, untouched.");
                }
            }
            wrapMode: Text.WordWrap
            color: "#aaaaaa"
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Monitoring only: whatever is selected here is left out of an audio render.")
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
