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
import "../Components"

// Sampling an instrument that is in the room: the recording lands on whichever pad is selected, so
// the strip sits with the waveform rather than on the pads themselves -- sixteen record buttons
// would say this is a per-pad thing, and it is not.
RowLayout {
    id: root

    property bool samplerDialogVisible: false

    spacing: 10

    AppButton {
        id: recordButton
        text: samplerController.recording ? qsTr("Stop") : qsTr("Record")
        enabled: samplerController.selectedPad >= 0 && inputCombo.count > 0
        onClicked: samplerController.recording ? samplerController.stopRecording() : samplerController.startRecording()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Records the chosen input onto the selected pad. Whatever the pad held is replaced.")
    }

    Rectangle {
        Layout.preferredWidth: 12
        Layout.preferredHeight: 12
        radius: 6
        color: samplerController.recording ? "#e0483c" : "#333"
        border.color: "#555"
        border.width: 1
    }

    Label {
        text: qsTr("Input")
    }

    // The same input the rest of the application records from, and the same setting: there is one
    // recorder and one persisted choice, so a picker of the sampler's own would change the live
    // device without saving it and leave Settings showing something else. Offered here because
    // choosing an input is part of sampling, owned there because that is where it belongs.
    ComboBox {
        id: inputCombo
        Layout.preferredWidth: 260
        textRole: "name"
        valueRole: "id"
        enabled: !samplerController.recording
        model: audioSettingsModel.inputDevices
        onActivated: audioSettingsModel.selectedInputDeviceId = currentValue
        Component.onCompleted: currentIndex = indexOfValue(audioSettingsModel.selectedInputDeviceId)
        Connections {
            target: audioSettingsModel
            function onInputDevicesChanged() {
                inputCombo.currentIndex = inputCombo.indexOfValue(audioSettingsModel.selectedInputDeviceId);
            }
            function onSelectedInputDeviceIdChanged() {
                inputCombo.currentIndex = inputCombo.indexOfValue(audioSettingsModel.selectedInputDeviceId);
            }
        }
    }

    Label {
        Layout.fillWidth: true
        elide: Text.ElideRight
        color: "#999"
        // Said plainly rather than hidden in a tooltip: a recording made before the project has
        // anywhere to live is only kept because saving writes it out, and that is worth knowing
        // before you record something you cannot do twice.
        text: samplerController.recording ? qsTr("Recording onto the selected pad…") : (editorService.currentFileName ? "" : qsTr("The project has not been saved, so recordings are kept only until you save it."))
    }

    AppButton {
        text: qsTr("Refresh")
        enabled: !samplerController.recording
        onClicked: audioSettingsModel.refreshInputDevices()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Looks for inputs again, for something plugged in since the dialog was opened.")
    }
}
