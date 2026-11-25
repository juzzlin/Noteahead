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
import Noteahead 1.0
import "../Components"

// Base for every rack effect's parameter dialog: the slot it edits, and the Ok/Cancel pair that
// makes an edit stick or puts back what the effect held when the dialog opened.
AnimatedDialog {
    id: root

    // Set by the rack dialog right before open(), so it is in place by the time the snapshot below
    // is taken.
    property int effectIndex: -1

    // Not a plain onAboutToShow: a dialog declaring one of its own would replace it
    Connections {
        target: root
        function onAboutToShow() {
            effectRackController.snapshotEffect(root.effectIndex);
        }
    }

    // The title as AnimatedDialog draws it, with the rack's on/off checkbox in the corner, so that
    // the effect can be compared with and without while its controls are in view
    header: Item {
        implicitHeight: Math.max(titleLabel.implicitHeight, enabledCheckBox.implicitHeight)
        visible: root.title !== ""

        Label {
            id: titleLabel
            anchors.left: parent.left
            anchors.right: enabledCheckBox.left
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            textFormat: Text.StyledText
            font.pixelSize: Theme.titlePixelSize
            font.bold: true
            color: Theme.headingText
            elide: Text.ElideRight
            leftPadding: 20
            rightPadding: 20
            topPadding: 14
            bottomPadding: 10
        }

        EffectEnabledCheckBox {
            id: enabledCheckBox
            effectIndex: root.effectIndex
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: titleLabel.verticalCenter
            width: 40
            height: 40
            Universal.theme: Universal.Dark
            Universal.accent: themeService.accentColor
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: 1
            color: Theme.separator
        }
    }

    // The handlers belong to the button box rather than to the dialog: only the Cancel button is
    // meant to revert, and Escape reaches Dialog.reject() without ever passing through the box.
    footer: DialogButtonBox {
        // The theme each effect dialog sets on its own root does not reach items built here, so
        // the buttons would come out in the light palette on a grey ground
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor

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
        onRejected: effectRackController.revertEffect(root.effectIndex)
    }
}
