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

    ComboBox {
        id: inputCombo
        Layout.preferredWidth: 260
        textRole: "name"
        valueRole: "id"
        enabled: !samplerController.recording
        onActivated: samplerController.setInputDevice(currentValue)
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

    function refreshInputs(): void {
        const devices = samplerController.inputDevices();
        inputCombo.model = devices;
        if (devices.length > 0 && inputCombo.currentIndex < 0) {
            inputCombo.currentIndex = 0;
        }
    }

    onSamplerDialogVisibleChanged: if (samplerDialogVisible) {
        refreshInputs();
    }
    Component.onCompleted: refreshInputs()
}
