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

// The bar carries two kinds of message that behave nothing alike, so it is split in two. On the
// left, guidance that stays true until the application state changes: no queue, no fade. On the
// right, transient notifications: queued and faded, as they always were. While playing, the note
// visualizer takes the whole bar and both halves step aside.
Rectangle {
    id: rootItem
    color: "black"
    property string _statusText: ""
    property var _statusQueue: []
    property bool _isDisplaying: false
    property string _tag: "BottomBar"
    readonly property string _tipText: qsTr("Press <b>ESC</b> to edit, <b>SPACE</b> to play, letter keys are notes")
    function setStatusText(text: string): void {
        uiLogger.debug(_tag, `Pushing new text '${text}'`);
        _statusQueue.push(text);
        if (!_isDisplaying) {
            _displayNextText();
        }
    }
    function setPosition(position: var): void {
        contentSwitcher.setPosition(position);
    }
    function _displayNextText(): void {
        if (_statusQueue.length > 0) {
            _isDisplaying = true;
            _statusText = _statusQueue.shift();
            _fadeOutText();
            uiLogger.debug(_tag, `Displaying text '${_statusText}'`);
        } else {
            _isDisplaying = false;
        }
    }
    function _fadeOutText(): void {
        fadeAnimation.running = true;
    }
    Item {
        id: contentSwitcher
        anchors.fill: parent
        function setPosition(position: var): void {
            noteVisualizer.setPosition(position);
        }
        NoteVisualizer {
            id: noteVisualizer
            anchors.fill: parent
            visible: UiService.isPlaying()
        }
        Item {
            id: tipArea
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * Constants.bottomBarTipAreaRatio
            visible: !UiService.isPlaying()
            Label {
                id: tipText
                anchors.fill: parent
                anchors.leftMargin: Constants.bottomBarTextMargin
                anchors.rightMargin: Constants.bottomBarTextMargin
                text: rootItem._tipText
                textFormat: Text.RichText
                color: "white"
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: rootItem.height * 0.5
            }
        }
        Rectangle {
            id: separator
            anchors.horizontalCenter: tipArea.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: Constants.bottomBarTextMargin / 2
            anchors.bottomMargin: Constants.bottomBarTextMargin / 2
            width: 1
            color: "white"
            opacity: Constants.bottomBarSeparatorOpacity
            visible: tipArea.visible
        }
        Item {
            id: notificationArea
            anchors.left: tipArea.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            Label {
                id: statusText
                anchors.fill: parent
                anchors.leftMargin: Constants.bottomBarTextMargin
                anchors.rightMargin: Constants.bottomBarTextMargin
                text: rootItem._statusText
                color: "white"
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: rootItem.height * 0.5
                opacity: 1
                // Messages emitted while playing are deliberately drained unseen: the visualizer
                // covers this area, and holding them back would build a backlog that dumps all at
                // once when playback stops.
                visible: !UiService.isPlaying()
                NumberAnimation {
                    id: fadeAnimation
                    target: statusText
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 2500
                    easing.type: Easing.InQuad
                    running: false
                    onRunningChanged: {
                        uiLogger.debug(_tag, `onRunningChanged: ${running} ${opacity}`);
                        if (!running) {
                            _displayNextText();
                        }
                    }
                }
            }
        }
    }
}
