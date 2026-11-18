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
import Noteahead 1.0
import "../Components"

AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Song Overview") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    // The controller builds only while this is open
    onAboutToShow: {
        songOverviewController.active = true;
        hover = ({});
        Qt.callLater(followPlayhead);
    }
    onClosed: songOverviewController.active = false

    property var hover: ({})
    //! 1 fits the whole song to the view's width; Ctrl+wheel zooms in from there.
    property real zoom: 1.0
    readonly property real maxZoom: 64.0

    // An internal device opens its own dialog, as the track header's device button does; any other
    // track opens its settings, which wait for the playback to stop like the header's button
    function openTrack(track: int): void {
        const portName = editorService.instrumentPortName(track);
        if (deviceService.isInternalDevice(portName)) {
            root.close();
            UiService.requestDeviceDialog(portName);
        } else if (!UiService.isPlaying()) {
            root.close();
            UiService.requestTrackSettingsDialog(track);
        }
    }

    function trackToolTip(track: int): string {
        const portName = editorService.instrumentPortName(track);
        return deviceService.isInternalDevice(portName) ? qsTr("Open %1").arg(portName) : qsTr("Open track settings");
    }

    function jumpTo(target: var): void {
        if (!target.valid) {
            return;
        }
        if (target.isTrack === true) {
            openTrack(target.track);
            return;
        }
        UiService.jumpToSongPosition(target.position);
        editorService.requestPosition(target.pattern, target.track, target.column, target.line, 0);
    }

    function maxContentX(): real {
        return Math.max(flickable.contentWidth - flickable.width, 0);
    }

    // Keeps the playhead in view when it plays past either edge
    function followPlayhead(): void {
        const x = renderer.trackColumnWidth + songOverviewController.playheadSongLine * renderer.pixelsPerLine;
        const left = flickable.contentX + renderer.trackColumnWidth;
        const right = flickable.contentX + flickable.width;
        if (x < left || x > right - 20) {
            flickable.contentX = Math.min(Math.max(x - renderer.trackColumnWidth - 40, 0), maxContentX());
        }
    }

    Connections {
        target: songOverviewController
        function onPlayheadChanged() {
            if (root.visible) {
                root.followPlayhead();
            }
        }
    }

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    footer: DialogButtonBox {
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    Item {
        anchors.fill: parent
        focus: true
        // The editor is out of reach under a modal dialog, so Space plays and stops from here
        Keys.onSpacePressed: event => {
            UiService.togglePlay();
            event.accepted = true;
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Label {
                    text: qsTr("The song as it plays: a lane per track, the play order across. Click a note to go to it in the editor, or a lane to go to that line on its track; double-click to also close this. Ctrl+wheel zooms.")
                    wrapMode: Text.WordWrap
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                AppButton {
                    text: playerService.isPlaying ? qsTr("Stop") : qsTr("Play")
                    implicitWidth: Constants.defaultButtonWidth
                    onClicked: UiService.togglePlay()
                }
            }

            Flickable {
                id: flickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: renderer.contentWidth
                contentHeight: renderer.contentHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                // As large as the view and moved with it: the renderer paints only what is in view,
                // and the overlays and the mouse work in view coordinates
                Item {
                    id: view
                    x: flickable.contentX
                    y: flickable.contentY
                    width: flickable.width
                    height: flickable.height

                    SongOverviewRenderer {
                        id: renderer
                        anchors.fill: parent
                        scrollX: flickable.contentX
                        scrollY: flickable.contentY
                        controller: songOverviewController
                        accentColor: themeService.accentColor
                        activeRow: songOverviewController.playheadRow
                        // Lanes grow to fill the view when there are few tracks
                        laneHeight: (flickable.height - stripHeight) / Math.max(songOverviewController.trackCount, 1) - laneSpacing
                        // The whole song fits the width at a zoom of 1
                        pixelsPerLine: (flickable.width - trackColumnWidth - 8) / Math.max(totalLines, 1) * root.zoom
                    }

                    // Over the play order strip: the active position's block and the playhead
                    Item {
                        x: renderer.trackColumnWidth
                        width: parent.width - x
                        height: renderer.stripHeight
                        clip: true
                        visible: songOverviewController.rowCount > 0
                        Rectangle {
                            x: songOverviewController.playheadSongLine * renderer.pixelsPerLine - flickable.contentX - 1
                            y: 2
                            width: 2
                            height: parent.height - 4
                            color: themeService.accentColor
                        }
                    }

                    // Over the lanes: a tint on the active position, the playhead, and the note a
                    // click would go to
                    Item {
                        x: renderer.trackColumnWidth
                        y: renderer.stripHeight
                        width: parent.width - x
                        // Down to the last lane's bottom, not the view's
                        height: Math.min(parent.height - y, songOverviewController.trackCount * (renderer.laneHeight + renderer.laneSpacing) - renderer.laneSpacing - flickable.contentY)
                        clip: true
                        visible: songOverviewController.rowCount > 0
                        Rectangle {
                            x: songOverviewController.rowStartLine(songOverviewController.playheadRow) * renderer.pixelsPerLine - flickable.contentX
                            width: songOverviewController.rowLineCount(songOverviewController.playheadRow) * renderer.pixelsPerLine
                            height: parent.height
                            color: themeService.accentColor
                            opacity: 0.06
                        }
                        Rectangle {
                            x: songOverviewController.playheadSongLine * renderer.pixelsPerLine - flickable.contentX - 1
                            width: 2
                            height: parent.height
                            color: themeService.accentColor
                        }
                        Rectangle {
                            x: root.hover.barX !== undefined ? root.hover.barX - flickable.contentX - renderer.trackColumnWidth - 1 : 0
                            y: root.hover.barY !== undefined ? root.hover.barY - flickable.contentY - renderer.stripHeight - 1 : 0
                            width: root.hover.barWidth !== undefined ? root.hover.barWidth + 2 : 0
                            height: root.hover.barHeight !== undefined ? root.hover.barHeight + 2 : 0
                            color: "transparent"
                            border.color: Theme.textBright
                            border.width: 1
                            visible: root.hover.barX !== undefined
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: root.hover.valid ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onPositionChanged: mouse => root.hover = renderer.hitTest(mouse.x, mouse.y)
                        onExited: root.hover = ({})
                        onClicked: mouse => root.jumpTo(renderer.hitTest(mouse.x, mouse.y))
                        onDoubleClicked: mouse => {
                            const target = renderer.hitTest(mouse.x, mouse.y);
                            if (target.valid && target.isTrack !== true) {
                                root.jumpTo(target);
                                root.close();
                            }
                        }
                        // Ctrl+wheel zooms about the line under the pointer; a plain wheel scrolls
                        onWheel: wheel => {
                            if (!(wheel.modifiers & Qt.ControlModifier)) {
                                wheel.accepted = false;
                                return;
                            }
                            const songLine = (wheel.x + flickable.contentX - renderer.trackColumnWidth) / renderer.pixelsPerLine;
                            const factor = wheel.angleDelta.y > 0 ? 1.25 : 0.8;
                            root.zoom = Math.min(Math.max(root.zoom * factor, 1.0), root.maxZoom);
                            flickable.contentX = Math.min(Math.max(songLine * renderer.pixelsPerLine + renderer.trackColumnWidth - wheel.x, 0), root.maxContentX());
                            root.hover = ({});
                        }
                        ToolTip.visible: containsMouse && (root.hover.isNote === true || root.hover.isTrack === true)
                        ToolTip.text: root.hover.isTrack === true ? root.trackToolTip(root.hover.track) : root.hover.isNote === true ? root.hover.trackName + "  " + root.hover.noteName + "  " + qsTr("Line") + " " + root.hover.line : ""
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                    }
                }
            }
        }
    }
}
