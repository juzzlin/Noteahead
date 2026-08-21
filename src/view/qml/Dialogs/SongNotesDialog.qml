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
import "../Components"

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Song notes") + "</strong>"
    modal: true
    width: parent ? parent.width * Constants.largeDialogScale : 700
    height: parent ? parent.height * Constants.largeDialogScale : 500
    readonly property string _tag: "SongNotesDialog"
    // An explicit button box rather than standardButtons, which stretches a lone button over the
    // whole dialog width.
    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                rootItem.accepted();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Close this dialog")
        }
    }
    function initialize(): void {
        uiLogger.info(_tag, "Initializing");
        // Assigned rather than bound, so that writing back on every keystroke cannot reset the
        // text under the cursor.
        notesTextArea.text = editorService.songNotes;
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        ScrollView {
            id: notesScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            TextArea {
                id: notesTextArea
                width: notesScrollView.availableWidth
                wrapMode: TextArea.Wrap
                selectByMouse: true
                font.family: "Monospace"
                placeholderText: qsTr("Anything worth keeping with the song: ideas, arrangement notes, what to fix next.")
                // Written back as it is typed rather than on close, so that closing the window
                // without pressing Ok cannot lose what was written.
                onTextChanged: editorService.songNotes = text
            }
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: themeService.accentColor
            text: qsTr("Saved inside the project file. Never written into rendered audio.")
        }
    }
}
