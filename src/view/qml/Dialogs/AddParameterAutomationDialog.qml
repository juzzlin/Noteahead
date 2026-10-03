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
import "../Components"

//! Adds an automation that drives an effect's parameter directly. See ParameterAutomationModel.
AnimatedDialog {
    id: rootItem
    modal: true
    width: parent ? parent.width * Constants.defaultDialogScale : 800
    height: parent ? parent.height * Constants.defaultDialogScale : 600
    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            // Nothing to add without something to drive
            enabled: model.hasTarget()
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
    function setTitle(text: string): void {
        title = "<strong>" + text + "</strong>";
    }
    //! Opens the form over the given lines, on the effects the project has right now.
    function openForLines(startLine: int, endLine: int): void {
        model.refreshTargets();
        model.reset();
        model.setStartLine(startLine);
        model.setEndLine(endLine);
        open();
    }
    property alias form: model
    contentItem: ScrollView {
        id: scrollView
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ParameterAutomationModel {
            id: model
            width: scrollView.availableWidth
        }
    }
}
