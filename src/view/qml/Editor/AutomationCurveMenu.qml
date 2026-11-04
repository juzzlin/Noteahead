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

//! What a right click on an automation's curve in the editor offers for that automation.
FittedMenu {
    id: root

    //! Which automation: a Constants' automationKind*, and its id within that kind.
    property int kind: Constants.automationKindMidiCc
    property int automationId: 0

    function openFor(kind: int, automationId: int, globalX: int, globalY: int): void {
        root.kind = kind;
        root.automationId = automationId;
        root.x = globalX;
        root.y = globalY;
        root.open();
    }

    MenuItem {
        text: qsTr("Edit...")
        onTriggered: UiService.requestEditAutomationDialogById(root.kind, root.automationId)
    }
    MenuItem {
        text: qsTr("Delete")
        onTriggered: editorService.requestAutomationDelete(root.kind, root.automationId)
    }
}
