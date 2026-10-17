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

AnimatedDialog {
    id: root
    title: applicationService.lfoDeviceName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 700

    //! Read when the dialog opens: what there is to modulate changes as devices and effects come and go
    property var targets: []
    onAboutToShow: targets = propertyService.automationTargets()

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: lfoController
            hostDialog: root
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => lfoController.accept()
        onRejected: () => lfoController.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        ScrollView {
            id: contentScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            // Two by two: four columns would leave each LFO's knobs too narrow to read
            GridLayout {
                width: contentScrollView.availableWidth
                columns: 2
                columnSpacing: 30
                rowSpacing: 20

                Repeater {
                    model: lfoController.lfos
                    delegate: LfoDialog_Lfo {
                        required property var modelData
                        lfo: modelData
                        targets: root.targets
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                    }
                }
            }
        }

        // Notes on the device's track restart the LFOs set to; this plays them here
        VirtualKeyboard {
            Layout.fillWidth: true
            onNoteOnRequested: note => lfoController.playNote(note, 1.0)
            onNoteOffRequested: note => lfoController.stopNote(note)
        }
    }
}
