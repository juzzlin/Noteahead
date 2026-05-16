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

Dialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("Reverb Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 600
    height: 450

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        RowLayout {
            spacing: 10
            Label { text: qsTr("Preset:") }
            ComboBox {
                model: effectRackController.reverbPresets()
                onActivated: index => effectRackController.applyReverbPreset(root.effectIndex, index)
                Layout.fillWidth: true
            }
        }

        GridLayout {
            columns: 3
            columnSpacing: 30
            rowSpacing: 20
            Layout.fillWidth: true

            Knob {
                label: qsTr("Size")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbSize") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbSize", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Decay")
                suffix: "ms"
                from: 0
                to: 10000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbDecay") * 10000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbDecay", v / 10000)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Damping")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbDamping") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbDamping", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Pre-Delay")
                suffix: "ms"
                from: 0
                to: 500
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbPreDelay") * 500;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbPreDelay", v / 500)
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Width")
                from: 0
                to: 2 * Constants.uiInternalScaling
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbWidth") * 2 * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbWidth", v / (2 * Constants.uiInternalScaling))
                Layout.fillWidth: true
            }

            Knob {
                label: qsTr("Mix")
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, "reverbMix") * Constants.uiInternalScaling;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, "reverbMix", v / Constants.uiInternalScaling)
                Layout.fillWidth: true
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    Connections {
        target: effectRackController
        function onParameterChanged(index, paramName) {
            if (index === root.effectIndex) {
                // This will force knobs to re-read values from the controller
                // In QML, accessing property triggers binding, but here we use a function.
                // We might need to use a model or properties for automatic updates.
                // For now, let's assume they update if we toggle visibility or similar.
                // Actually, since we use parameterValue() function, we might need a way to notify QML.
            }
        }
    }
}
