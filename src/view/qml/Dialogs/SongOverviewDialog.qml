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
        cancelNoteDrag();
        setHover({});
        Qt.callLater(followPlayhead);
    }
    onClosed: songOverviewController.active = false

    property var hover: ({})
    //! A note being dragged: what was pressed, how, and where it would go. Null when none is.
    property var noteDrag: null
    //! The release that ended a drag is not also a click
    property bool dragJustEnded: false
    readonly property real dragThreshold: 4
    //! The view being dragged by a press off the notes: where it started, in the Flickable's
    //! coordinates, which stay put while the view scrolls. Null when it is not.
    property var pan: null
    readonly property real lengthHandleWidth: 6
    //! 1 fits the whole song to the view's width; Ctrl+wheel zooms in from there.
    property real zoom: 1.0
    readonly property real maxZoom: 64.0
    //! The track indices in focus: only their lanes are shown, at full height. Empty shows all.
    property var focusedTracks: []
    //! The tracks whose focus buttons are lit: the focused ones, and those Ctrl+clicked to join them.
    property var markedTracks: []

    function sameTracks(a: var, b: var): bool {
        const sorted = list => list.slice().sort((x, y) => x - y).join(",");
        return sorted(a) === sorted(b);
    }

    // A click focuses the tracks marked so far plus this one, or takes a focused track out of focus;
    // a Ctrl+click only marks or unmarks it, since focusing at once would hide the tracks still to pick
    function toggleFocus(track: int, pickOnly: bool): void {
        const marked = markedTracks.slice();
        const index = marked.indexOf(track);
        if (pickOnly) {
            if (index >= 0) {
                marked.splice(index, 1);
            } else {
                marked.push(track);
            }
            markedTracks = marked;
            return;
        }
        if (index < 0) {
            marked.push(track);
        } else if (sameTracks(marked, focusedTracks)) {
            marked.splice(index, 1);
        }
        markedTracks = marked;
        focusedTracks = marked.slice();
        flickable.contentY = 0;
        setHover({});
    }

    function focusToolTip(track: int): string {
        return focusedTracks.indexOf(track) >= 0 && sameTracks(markedTracks, focusedTracks) ? qsTr("Take this track out of focus") : qsTr("Focus on this track: only the tracks in focus are shown, at full height. Ctrl+click to pick several.");
    }

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

    //! Whether content x @p contentX is on the right end of @p target's bar, which drags its length
    function isOnNoteEnd(target: var, contentX: real): bool {
        const right = target.barX + target.barWidth;
        return target.barWidth >= 3 * lengthHandleWidth / 2 && contentX >= right - lengthHandleWidth && contentX <= right + 2;
    }

    function pixelsPerSemitone(lane: int): real {
        return renderer.pitchY(lane, 60) - renderer.pitchY(lane, 61);
    }

    function startNoteDrag(target: var, x: real, y: real): void {
        noteDrag = {
            "target": target,
            "resize": isOnNoteEnd(target, x + flickable.contentX),
            "pressX": x,
            "pressY": y,
            "moved": false,
            "lineDelta": 0,
            "semitoneDelta": 0,
            "endLine": target.noteStartLine + target.noteLengthLines,
            // Every position that plays the pattern shows the edit
            "copies": songOverviewController.rowStartLinesOfPattern(target.pattern).map(start => start - songOverviewController.rowStartLine(target.row))
        };
        flickable.interactive = false;
    }

    function updateNoteDrag(x: real, y: real): void {
        const drag = noteDrag;
        if (!drag.moved && Math.abs(x - drag.pressX) < dragThreshold && Math.abs(y - drag.pressY) < dragThreshold) {
            return;
        }
        drag.moved = true;
        const lines = (x - drag.pressX) / renderer.pixelsPerLine;
        if (drag.resize) {
            // As far as the editor lets it go: not before its own line, nor past the next note
            drag.endLine = Math.min(Math.max(drag.target.line + 1, Math.round(drag.target.noteStartLine + drag.target.noteLengthLines + lines)), drag.target.noteLimitLine);
        } else {
            drag.lineDelta = Math.round(lines);
            drag.semitoneDelta = Math.round((drag.pressY - y) / pixelsPerSemitone(drag.target.lane));
        }
        noteDrag = Object.assign({}, drag);
    }

    function endNoteDrag(): void {
        const drag = noteDrag;
        noteDrag = null;
        flickable.interactive = true;
        if (!drag || !drag.moved) {
            return;
        }
        dragJustEnded = true;
        const t = drag.target;
        if (drag.resize) {
            editorService.requestNoteLength(t.pattern, t.track, t.column, t.line, drag.endLine);
        } else if (drag.lineDelta !== 0 || drag.semitoneDelta !== 0) {
            editorService.requestNoteMove(t.pattern, t.track, t.column, t.line, drag.lineDelta, drag.semitoneDelta);
        }
        setHover({});
    }

    function startPan(point: point): void {
        pan = { x: point.x, y: point.y, contentX: flickable.contentX, contentY: flickable.contentY, moved: false };
        flickable.interactive = false;
    }

    function updatePan(point: point): void {
        const dx = point.x - pan.x;
        const dy = point.y - pan.y;
        if (!pan.moved && Math.abs(dx) < dragThreshold && Math.abs(dy) < dragThreshold) {
            return;
        }
        if (!pan.moved) {
            pan = Object.assign({}, pan, { moved: true });
            setHover({});
        }
        flickable.contentX = Math.min(Math.max(pan.contentX - dx, 0), maxContentX());
        flickable.contentY = Math.min(Math.max(pan.contentY - dy, 0), Math.max(flickable.contentHeight - flickable.height, 0));
    }

    function endPan(): void {
        if (pan && pan.moved) {
            dragJustEnded = true;
        }
        pan = null;
        flickable.interactive = true;
    }

    function cancelNoteDrag(): void {
        noteDrag = null;
        flickable.interactive = true;
    }

    //! What is under the pointer, and the automation curve drawn thicker if it is one
    function setHover(target: var): void {
        hover = target;
        renderer.setHoveredCurve(target.isCurve === true ? target.curveKind : -1, target.isCurve === true ? target.automationId : -1);
    }

    //! A curve's own name if it was given one, otherwise what it moves
    function curveName(target: var): string {
        if (target.comment !== "") {
            return target.comment;
        }
        if (target.controller >= 0) {
            return "CC " + target.controller;
        }
        return target.curveKind === Constants.automationKindPitchBend ? qsTr("Pitch Bend") : target.parameter;
    }

    // Curves only while stopped, as in the editor: the automation dialogs do not open during playback
    function hitTest(x: real, y: real): var {
        return renderer.hitTest(x, y, !UiService.isPlaying());
    }

    function jumpTo(target: var): void {
        if (!target.valid) {
            return;
        }
        if (target.isTrack === true) {
            openTrack(target.track);
            return;
        }
        if (target.isCurve === true) {
            root.close();
            UiService.requestEditAutomationDialogById(target.curveKind, target.automationId);
            return;
        }
        UiService.jumpToSongPosition(target.position);
        editorService.requestPosition(target.pattern, target.track, target.column, target.line, 0);
    }

    function maxContentX(): real {
        return Math.max(flickable.contentWidth - flickable.width, 0);
    }

    // Keeps the playhead in view when it plays past either edge, but not under a drag of the view
    function followPlayhead(): void {
        if (pan !== null) {
            return;
        }
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
        function onNotesStarted(notes) {
            if (!root.visible) {
                return;
            }
            for (const note of notes) {
                // A dense chord on every lane is still only so many circles
                if (flashLayer.children.length >= root.maxFlashes) {
                    break;
                }
                noteFlashComponent.createObject(flashLayer, {
                    "x": renderer.songLineX(note.songLine),
                    "y": renderer.pitchY(note.lane, note.pitch),
                    "tint": renderer.trackColor(note.lane),
                    "strength": note.velocity / 127
                });
            }
        }
    }

    readonly property int maxFlashes: 96

    // A circle where the playhead meets a note: a dot that fades and a ring that spreads, both
    // bigger, brighter and longer for a louder note
    Component {
        id: noteFlashComponent
        Item {
            id: flash
            property color tint: themeService.accentColor
            property real strength: 1.0
            property real progress: 0.0
            readonly property real maxDiameter: 10 + 34 * strength
            Rectangle {
                anchors.centerIn: parent
                width: flash.maxDiameter * (0.25 + 0.2 * (1 - flash.progress))
                height: width
                radius: width / 2
                color: flash.tint
                opacity: (0.35 + 0.65 * flash.strength) * (1 - flash.progress)
            }
            Rectangle {
                anchors.centerIn: parent
                width: flash.maxDiameter * (0.25 + 0.75 * flash.progress)
                height: width
                radius: width / 2
                color: "transparent"
                border.color: flash.tint
                border.width: 1 + 2 * flash.strength
                opacity: (0.4 + 0.6 * flash.strength) * (1 - flash.progress)
            }
            NumberAnimation on progress {
                from: 0
                to: 1
                duration: 350 + 350 * flash.strength
                easing.type: Easing.OutCubic
                onFinished: flash.destroy()
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
                    text: qsTr("The song as it plays: a lane per track, the play order across. Click a note to go to it in the editor, or a lane to go to that line on its track; double-click to also close this. Ctrl+wheel zooms.") + " " + qsTr("The button by a track's name focuses on it.") + " " + qsTr("Drag a note to move it, or its end to change its length.") + " " + qsTr("Right-click a note to delete it.")
                    wrapMode: Text.WordWrap
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                CheckBox {
                    text: qsTr("Automations")
                    checked: settingsService.songOverviewAutomationsVisible
                    onToggled: settingsService.songOverviewAutomationsVisible = checked
                    Universal.theme: Universal.Dark
                    Universal.accent: themeService.accentColor
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Draw the automations over the notes of their tracks, as the editor's curves")
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
                        automationsVisible: settingsService.songOverviewAutomationsVisible
                        automationCurveColors: themeService.automationCurveColors
                        activeRow: songOverviewController.playheadRow
                        // Lanes grow to fill the view when there are few tracks
                        laneHeight: (flickable.height - stripHeight) / Math.max(songOverviewController.trackCount, 1) - laneSpacing
                        focusedTracks: root.focusedTracks
                        markedTracks: root.markedTracks
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
                        height: Math.min(parent.height - y, renderer.contentHeight - renderer.stripHeight - renderer.laneSpacing - flickable.contentY)
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
                        // The note flashes, placed in content coordinates
                        Item {
                            id: flashLayer
                            x: -flickable.contentX - renderer.trackColumnWidth
                            y: -flickable.contentY - renderer.stripHeight
                        }
                        Rectangle {
                            x: root.hover.barX !== undefined ? root.hover.barX - flickable.contentX - renderer.trackColumnWidth - 1 : 0
                            y: root.hover.barY !== undefined ? root.hover.barY - flickable.contentY - renderer.stripHeight - 1 : 0
                            width: root.hover.barWidth !== undefined ? root.hover.barWidth + 2 : 0
                            height: root.hover.barHeight !== undefined ? root.hover.barHeight + 2 : 0
                            color: "transparent"
                            border.color: Theme.textBright
                            border.width: 1
                            visible: root.hover.barX !== undefined && root.noteDrag === null
                        }
                        // Where the dragged note would go, at every position that plays its pattern
                        Repeater {
                            model: root.noteDrag !== null && root.noteDrag.moved ? root.noteDrag.copies : []
                            Rectangle {
                                readonly property var drag: root.noteDrag
                                readonly property var target: drag ? drag.target : ({})
                                readonly property bool isDragged: modelData === 0
                                x: drag ? target.barX + (modelData + drag.lineDelta) * renderer.pixelsPerLine - flickable.contentX - renderer.trackColumnWidth - 1 : 0
                                y: drag ? target.barY - drag.semitoneDelta * root.pixelsPerSemitone(target.lane) - flickable.contentY - renderer.stripHeight - 1 : 0
                                width: drag ? (drag.resize ? Math.max((drag.endLine - target.noteStartLine) * renderer.pixelsPerLine, 1) : target.barWidth) + 2 : 0
                                height: drag ? target.barHeight + 2 : 0
                                color: isDragged ? Qt.rgba(1, 1, 1, 0.25) : "transparent"
                                border.color: Theme.textBright
                                border.width: 1
                                opacity: isDragged ? 1.0 : 0.5
                            }
                        }
                    }

                    // A right-clicked note's menu, while stopped
                    Menu {
                        id: noteMenu
                        property var target: ({})
                        Universal.theme: Universal.Dark
                        Universal.accent: themeService.accentColor
                        // Not a plain MenuItem: its own click handling can drop clicks while the
                        // audio engine runs, which MenuItemDelegate works around
                        MenuItemDelegate {
                            text: qsTr("Delete")
                            onTriggered: {
                                const t = noteMenu.target;
                                if (!UiService.isPlaying() && t.isNote === true) {
                                    editorService.requestNoteDelete(t.pattern, t.track, t.column, t.line);
                                }
                                root.setHover({});
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: {
                            if (root.noteDrag !== null) {
                                return root.noteDrag.resize ? Qt.SizeHorCursor : Qt.SizeAllCursor;
                            }
                            if (root.pan !== null && root.pan.moved) {
                                return Qt.ClosedHandCursor;
                            }
                            if (root.hover.isNote === true && !UiService.isPlaying() && root.isOnNoteEnd(root.hover, mouseX + flickable.contentX)) {
                                return Qt.SizeHorCursor;
                            }
                            return root.hover.valid ? Qt.PointingHandCursor : Qt.ArrowCursor;
                        }
                        preventStealing: root.noteDrag !== null || root.pan !== null
                        // A note is dragged to move it, or by its end to change its length, while stopped;
                        // anywhere else, or while playing, a drag moves the view
                        onPressed: mouse => {
                            root.dragJustEnded = false;
                            if (UiService.isPlaying()) {
                                if (mouse.button === Qt.LeftButton) {
                                    root.startPan(mapToItem(flickable, mouse.x, mouse.y));
                                }
                                return;
                            }
                            if (mouse.button === Qt.RightButton) {
                                const noteTarget = renderer.hitTest(mouse.x, mouse.y, false);
                                if (noteTarget.isNote === true) {
                                    noteMenu.target = noteTarget;
                                    noteMenu.popup();
                                }
                                return;
                            }
                            const target = root.hitTest(mouse.x, mouse.y);
                            if (target.isNote === true) {
                                root.startNoteDrag(target, mouse.x, mouse.y);
                            } else if (target.isTrack !== true) {
                                root.startPan(mapToItem(flickable, mouse.x, mouse.y));
                            }
                        }
                        onReleased: {
                            root.endNoteDrag();
                            root.endPan();
                        }
                        onCanceled: {
                            root.cancelNoteDrag();
                            root.endPan();
                        }
                        onPositionChanged: mouse => {
                            if (root.noteDrag !== null) {
                                root.updateNoteDrag(mouse.x, mouse.y);
                            } else if (root.pan !== null) {
                                root.updatePan(mapToItem(flickable, mouse.x, mouse.y));
                            } else {
                                root.setHover(root.hitTest(mouse.x, mouse.y));
                            }
                        }
                        onExited: {
                            if (root.noteDrag === null && root.pan === null) {
                                root.setHover({});
                            }
                        }
                        onClicked: mouse => {
                            if (mouse.button !== Qt.LeftButton || root.dragJustEnded) {
                                return;
                            }
                            const target = root.hitTest(mouse.x, mouse.y);
                            if (target.isFocusButton === true) {
                                root.toggleFocus(target.track, (mouse.modifiers & Qt.ControlModifier) !== 0);
                            } else {
                                root.jumpTo(target);
                            }
                        }
                        onDoubleClicked: mouse => {
                            if (mouse.button !== Qt.LeftButton) {
                                return;
                            }
                            // A curve's click has already closed this for its automation
                            const target = root.hitTest(mouse.x, mouse.y);
                            if (target.valid && target.isTrack !== true && target.isCurve !== true) {
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
                            root.setHover({});
                        }
                        ToolTip.visible: containsMouse && root.noteDrag === null && root.pan === null && (root.hover.isNote === true || root.hover.isTrack === true || root.hover.isCurve === true)
                        ToolTip.text: root.hover.isFocusButton === true ? root.focusToolTip(root.hover.track) : root.hover.isTrack === true ? root.trackToolTip(root.hover.track) : root.hover.isCurve === true ? root.hover.trackName + "  " + root.curveName(root.hover) : root.hover.isNote === true ? root.hover.trackName + "  " + root.hover.noteName + "  " + qsTr("Line") + " " + root.hover.line : ""
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                    }
                }
            }
        }
    }
}
