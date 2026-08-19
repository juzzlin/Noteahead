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
import QtQuick.Dialogs
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: qsTr("Save unsaved changes?")
    modal: true
    readonly property string _tag: "UnsavedChangesDialog"
    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Yes")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes accepted");
                applicationService.acceptUnsavedChangesDialog();
                close();
            }
        }
        AppButton {
            text: qsTr("Close without saving")
            implicitWidth: Math.max(Constants.defaultButtonWidth, implicitContentWidth + leftPadding + rightPadding)
            DialogButtonBox.buttonRole: DialogButtonBox.NoRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes discarded");
                applicationService.discardUnsavedChangesDialog();
                close();
            }
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes rejected");
                applicationService.rejectUnsavedChangesDialog();
                close();
            }
        }
    }
}
