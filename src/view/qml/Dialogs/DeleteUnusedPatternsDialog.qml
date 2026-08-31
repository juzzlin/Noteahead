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
    // Sized here rather than in Main.qml, per the dialog sizing rules. Without an explicit width the
    // dialog took its width from the wrapping label, whose own width came back from the dialog: a
    // binding loop that only showed itself once a language change forced the text to be laid out
    // again.
    width: parent ? parent.width * Constants.defaultDialogScale : 600
    readonly property string _tag: "DeleteUnusedPatternsDialog"

    Label {
        // Anchored, not bound to parent.width, so the size flows one way only.
        anchors.fill: parent
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
