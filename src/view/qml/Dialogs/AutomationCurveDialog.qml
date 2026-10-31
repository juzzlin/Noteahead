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
import "../Components"

// Draws the curve of one automation of an edit dialog. Opened on top of that dialog and written into
// its row when accepted, so the edit dialog's own Cancel still takes it all back.
AnimatedDialog {
    id: rootItem
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600
    standardButtons: Dialog.Ok | Dialog.Cancel

    //! Which edit dialog the row is in: "midiCc", "pitchBend" or "parameter".
    property string kind: "parameter"
    //! The row of that dialog's model the curve is written into.
    property int targetRow: -1
    //! Reads a value out the way the automation's own controls do.
    property var formatValue: value => value.toFixed(2)

    function setTitle(text: string): void {
        title = "<strong>" + text + "</strong>";
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        AutomationCurveView {
            editor: automationCurveEditor
            linesPerBeat: editorService.linesPerBeat
            formatValue: rootItem.formatValue
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label {
                text: qsTr("Click to add a node and drag to move it, right-click to delete it. Hold Shift to place it between lines.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("Undo")
                enabled: automationCurveEditor.canUndo
                onClicked: automationCurveEditor.undo()
            }
        }
    }
}
