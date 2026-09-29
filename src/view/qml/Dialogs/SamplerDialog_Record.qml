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
ColumnLayout {
    id: root

    property bool samplerDialogVisible: false

    //! True while the audio devices are being enumerated, which is a hardware probe and can take long
    //! enough to be worth saying something about.
    property bool inputsPending: false

    spacing: 4

    //! Asks for the input list, off the caller's own frame.
    //!
    //! Enumerating devices blocks the thread that asks, so doing it straight from the dialog's open
    //! handler froze the window before it had painted. The timer hands the frame back first, which is
    //! what lets the indicator below be seen at all.
    function refreshInputs() {
        root.inputsPending = true;
        inputRefreshTimer.restart();
    }

    Timer {
        id: inputRefreshTimer
        interval: 1
        repeat: false
        onTriggered: {
            audioSettingsModel.refreshInputDevices();
            root.inputsPending = false;
        }
    }

    // The whole width: there is more here than half a dialog holds, and the status line below has a
    // row of its own rather than taking the other half of this one.
    RowLayout {
        id: controls

        Layout.fillWidth: true
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
            // A quarter of the strip and no more. Device names run long, and this is the only control
            // here that can absorb width without needing it.
            Layout.maximumWidth: root.width * 0.25
            textRole: "name"
            valueRole: "id"
            enabled: !samplerController.recording && !root.inputsPending
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

        BusyIndicator {
            running: root.inputsPending
            visible: root.inputsPending
            implicitWidth: inputCombo.implicitHeight
            implicitHeight: inputCombo.implicitHeight
            Layout.preferredWidth: implicitWidth
        }

        AppButton {
            text: qsTr("Refresh")
            enabled: !samplerController.recording && !root.inputsPending
            onClicked: root.refreshInputs()
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Looks for inputs again, for something plugged in since the dialog was opened.")
        }

        // Counts you in before the take and, unless told otherwise, keeps time through it.
        CheckBox {
            id: metronomeCheckBox
            text: qsTr("Metronome")
            enabled: !samplerController.recording
            checked: samplerController.metronomeEnabled
            onToggled: samplerController.metronomeEnabled = checked
            contentItem: Label {
                text: metronomeCheckBox.text
                color: "white"
                verticalAlignment: Text.AlignVCenter
                leftPadding: metronomeCheckBox.indicator.width + metronomeCheckBox.spacing
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Clicks the song's tempo. Recording starts during the count-in, and the take is trimmed to begin on the downbeat, so nothing played early is lost.")
        }

        SpinBox {
            id: preCountSpinBox
            from: 0
            to: 8
            value: samplerController.preCountBars
            editable: false
            enabled: !samplerController.recording && samplerController.metronomeEnabled
            implicitWidth: 130
            onValueModified: samplerController.preCountBars = value
            textFromValue: (value, locale) => value === 0 ? qsTr("No count") : qsTr("%n bar(s)", "", value)
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How many bars are counted before the take begins.")
        }

        CheckBox {
            id: clickDuringTakeCheckBox
            text: qsTr("Click during take")
            enabled: !samplerController.recording && samplerController.metronomeEnabled
            checked: samplerController.clickDuringTake
            onToggled: samplerController.clickDuringTake = checked
            contentItem: Label {
                text: clickDuringTakeCheckBox.text
                color: "white"
                verticalAlignment: Text.AlignVCenter
                leftPadding: clickDuringTakeCheckBox.indicator.width + clickDuringTakeCheckBox.spacing
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Keeps clicking through the take. Turn it off when the click is coming back into the input, as it does when a mixer feeds the output back: the count-in is trimmed off the take, so only a click heard during the take ends up in what the pad plays.")
        }

        // Takes whatever the capped input box leaves, so the controls stay grouped at the left
        // instead of being stretched apart across the strip.
        Item {
            Layout.fillWidth: true
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
