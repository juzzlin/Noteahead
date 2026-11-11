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
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

// The five sources, each with its volume and its switch. Ext In is the output fed back in, as on
// the instrument with nothing patched: turned up, it drives the mixer into overload.
ColumnLayout {
    id: root

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    RowLayout {
        Layout.topMargin: 10
        SectionHeading {
            text: qsTr("Mixer")
        }
        LfoLed {
            controller: synthDController
            lfoIndex: 1
            ToolTip.visible: overloadHover.hovered
            ToolTip.text: qsTr("Overload: the mix is driving the filter into distortion")
            HoverHandler {
                id: overloadHover
            }
        }
    }
    // The oscillators on the first row, the noise and the feedback on the second
    GridLayout {
        columns: 3
        columnSpacing: 20
        Layout.fillWidth: true
        Repeater {
            model: {
                languageService.activeLanguage;
                return [{
                        "label": qsTr("OSC 1"),
                        "volume": "osc1Volume",
                        "on": "osc1On"
                    }, {
                        "label": qsTr("OSC 2"),
                        "volume": "osc2Volume",
                        "on": "osc2On"
                    }, {
                        "label": qsTr("OSC 3"),
                        "volume": "osc3Volume",
                        "on": "osc3On"
                    }, {
                        "label": qsTr("Noise"),
                        "volume": "noiseVolume",
                        "on": "noiseOn"
                    }, {
                        "label": qsTr("Ext In"),
                        "volume": "extVolume",
                        "on": "extOn"
                    }];
            }
            ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                Knob {
                    label: modelData.label
                    mapping: "value"
                    suffix: ""
                    mapMin: 0
                    mapMax: 10
                    value: synthDController[modelData.volume]
                    onMoved: v => synthDController[modelData.volume] = v
                    Layout.fillWidth: true
                }
                CheckBox {
                    text: qsTr("On")
                    checked: synthDController[modelData.on]
                    onToggled: synthDController[modelData.on] = checked
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Noise")
        }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("White"), qsTr("Pink")];
            }
            currentIndex: synthDController.noiseColour
            onActivated: i => synthDController.noiseColour = i
            Layout.fillWidth: true
        }
    }
}
