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

// Sampling an instrument that is in the room: the recording lands on whichever pad is selected, so
// the strip sits with the waveform rather than on the pads themselves -- sixteen record buttons
// would say this is a per-pad thing, and it is not.
RowLayout {
    id: root

    property bool samplerDialogVisible: false
    //! What the controls span, so they can be lined up with the pad matrix below them. Zero lets
    //! them take whatever they need.
    property real controlsWidth: 0

    spacing: 10

    // Sized to the pad matrix underneath, so the strip reads as belonging to it rather than
    // floating above it at a width of its own.
    RowLayout {
        id: controls

        // preferredWidth alone loses to what the children say they need -- a long device name in
        // the box is enough to push the group past its half -- so the cap has to be given as well,
        // and the box has to be allowed to shrink under its own implicit width.
        Layout.fillWidth: false
        Layout.preferredWidth: root.controlsWidth > 0 ? root.controlsWidth : implicitWidth
        Layout.maximumWidth: root.controlsWidth > 0 ? root.controlsWidth : Number.POSITIVE_INFINITY
        spacing: 10

        // The same round, red-rimmed record button the song recorder has in the editor: pressing
        // record is pressing record, wherever you happen to be.
        Button {
            id: recordButton

            Layout.preferredHeight: inputCombo.implicitHeight
            Layout.preferredWidth: height

            enabled: samplerController.selectedPad >= 0 && inputCombo.count > 0
            opacity: enabled ? 1.0 : 0.5
            focusPolicy: Qt.NoFocus
            onClicked: samplerController.recording ? samplerController.stopRecording() : samplerController.startRecording()

            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: samplerController.recording ? qsTr("Stops recording and puts what was recorded on the pad.") : qsTr("Records the chosen input onto the selected pad. Whatever the pad held is replaced.")

            background: Rectangle {
                id: recordBackground
                radius: height / 2
                color: samplerController.recording ? "#440000" : "#333333"
                border.color: samplerController.recording ? "#FF0000" : "#555555"
                border.width: 1

                // Breathing while it is actually recording, which is what tells it apart from the
                // song's switch: that one is armed and waiting, this one is running.
                SequentialAnimation on opacity {
                    running: samplerController.recording
                    loops: Animation.Infinite
                    alwaysRunToEnd: true
                    NumberAnimation {
                        to: 0.45
                        duration: 600
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: 1.0
                        duration: 600
                        easing.type: Easing.InOutSine
                    }
                    onRunningChanged: if (!running) {
                        recordBackground.opacity = 1.0;
                    }
                }
            }

            contentItem: Item {
                Image {
                    source: "../Graphics/record.png"
                    width: parent.height * 0.8
                    height: width
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    x: Math.floor((parent.width - width) / 2)
                    y: Math.floor((parent.height - height) / 2)
                }
            }
        }

        // The same input the rest of the application records from, and the same setting: there is
        // one recorder and one persisted choice, so a picker of the sampler's own would change the
        // live device without saving it and leave Settings showing something else. Offered here
        // because choosing an input is part of sampling, owned there because that is where it
        // belongs.
        ComboBox {
            id: inputCombo

            Layout.fillWidth: true
            Layout.minimumWidth: 60
            textRole: "name"
            valueRole: "id"
            enabled: !samplerController.recording
            model: audioSettingsModel.inputDevices
            onActivated: audioSettingsModel.selectedInputDeviceId = currentValue
            Component.onCompleted: currentIndex = indexOfValue(audioSettingsModel.selectedInputDeviceId)

            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Which input is sampled. The same one the rest of the application records from, so changing it here changes it in Settings too.")

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

        AppButton {
            text: qsTr("Refresh")
            enabled: !samplerController.recording
            onClicked: audioSettingsModel.refreshInputDevices()
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Looks for inputs again, for something plugged in since the dialog was opened.")
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
}
