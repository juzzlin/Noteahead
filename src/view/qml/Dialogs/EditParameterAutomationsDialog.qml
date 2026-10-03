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
import ".."

AnimatedDialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: parent ? parent.width * Constants.defaultDialogScale : 800
    height: parent ? parent.height * Constants.defaultDialogScale : 600
    //! Read once per opening, for the rows to name their effects by. See EditParameterAutomationsDelegate.
    property var targets: []
    function setTitle(text: string): void {
        title = "<strong>" + text + "</strong>";
    }
    onAboutToShow: targets = propertyService.automationTargets()
    contentItem: ScrollView {
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ListView {
            id: parameterAutomationsList
            model: parameterAutomationsModel
            spacing: 10
            clip: true
            delegate: EditParameterAutomationsDelegate {
                width: parameterAutomationsList.width
                height: implicitHeight
                targets: rootItem.targets
            }
        }
    }
    Text {
        text: qsTr("No parameter automations added")
        anchors.centerIn: parent
        color: "white"
        visible: !parameterAutomationsList.count
    }
}
