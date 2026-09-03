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
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: qsTr("Delete unused patterns")
    modal: true
    // A flat width rather than a fraction of the window, which is the usual idiom here. This dialog was
    // the only one taking a width from its parent while leaving its height to the content, and that
    // pair loops through the popup's own geometry: Qt reports it against implicitHeight at startup,
    // before the dialog is ever opened. Either half on its own is harmless -- the dialogs that set both
    // a width and a height are fine with parent, and so is a width that does not come from parent.
    //
    // One sentence and two buttons do not need a height of their own, nor seven tenths of the window,
    // so the width is the half that gives way.
    width: 600
    readonly property string _tag: "DeleteUnusedPatternsDialog"

    Label {
        // Width from the dialog, height from the text, so the size flows one way only. Filling the
        // parent instead makes the dialog's implicitHeight depend on the very item it is sizing, which
        // Qt reports as a binding loop on implicitHeight the first time the dialog is laid out.
        width: rootItem.availableWidth
        text: qsTr("Are you sure you want to delete all patterns that are not used in the play order?")
        wrapMode: Label.WordWrap
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Yes")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                uiLogger.info(rootItem._tag, "Deletion confirmed");
                rootItem.accept();
            }
        }
        AppButton {
            text: qsTr("No")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                uiLogger.info(rootItem._tag, "Deletion rejected");
                rootItem.reject();
            }
        }
    }

    onAccepted: {
        UiService.confirmDeleteUnusedPatterns();
    }
}
