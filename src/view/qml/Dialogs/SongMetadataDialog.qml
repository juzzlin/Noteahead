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
    title: "<strong>" + qsTr("Song metadata") + "</strong>"
    modal: true
    width: parent ? parent.width * Constants.largeDialogScale : 700
    height: parent ? parent.height * Constants.defaultDialogScale : 500
    readonly property string _tag: "SongMetadataDialog"
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
    }
    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 10
            GroupBox {
                title: qsTr("Song")
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    rowSpacing: 6
                    columnSpacing: 12
                    width: parent.width
                    Label {
                        text: qsTr("Title")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataTitle
                        onTextEdited: editorService.songMetadataTitle = text
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Composer")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataComposer
                        onTextEdited: editorService.songMetadataComposer = text
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Artist")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataArtist
                        onTextEdited: editorService.songMetadataArtist = text
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Date")
                    }
                    DateField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataDate
                        onEdited: text => editorService.songMetadataDate = text
                    }
                    Label {
                        text: qsTr("Genre")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataGenre
                        onTextEdited: editorService.songMetadataGenre = text
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Comment")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataComment
                        onTextEdited: editorService.songMetadataComment = text
                        Keys.onReturnPressed: focus = false
                    }
                }
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: themeService.accentColor
                text: qsTr("Describes the song itself and is saved in the project. A rendered audio file is tagged with these unless the render dialog says otherwise.")
            }
            // Keeps the group box at the top instead of stretching it over the dialog's height.
            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}
