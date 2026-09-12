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
import QtQuick.Layouts
import Noteahead 1.0

//! Asks for the name a patch is to be saved under. The controller whose patch it is travels with
//! the request, so one instance serves every device dialog.
AnimatedDialog {
    id: rootItem

    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: "<strong>" + qsTr("Save Preset") + "</strong>"
    width: Constants.smallDialogWidth

    //! The controller that will be asked to save. Set by whoever opens the dialog.
    property var targetController: null

    function presetName() {
        return nameField.text.trim();
    }
    function setPresetName(name) {
        nameField.text = name;
    }

    onOpened: {
        updateAcceptability();
        nameField.selectAll();
        nameField.forceActiveFocus();
    }

    //! An empty name has nothing to save under, and Ok has to say so before it is pressed. Set
    //! rather than bound: standardButton() is a lookup, so a binding on it would be evaluated once
    //! while the footer is still being built and never again.
    function updateAcceptability() {
        const okButton = standardButton(Dialog.Ok);
        if (okButton) {
            okButton.enabled = presetName() !== "";
        }
    }

    contentItem: ColumnLayout {
        spacing: 10
        Label {
            text: qsTr("Preset name:")
            Layout.fillWidth: true
        }
        TextField {
            id: nameField
            selectByMouse: true
            Layout.fillWidth: true
            onTextChanged: rootItem.updateAcceptability()
            Keys.onReturnPressed: {
                if (rootItem.presetName() !== "") {
                    rootItem.accept();
                }
            }
        }
    }
}
