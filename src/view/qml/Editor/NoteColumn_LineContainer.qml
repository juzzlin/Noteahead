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
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import ".."

Item {
    id: rootItem
    clip: true
    signal leftClicked(int lineIndex, int x, int y)
    signal rightClicked(int lineIndex, int x, int y)
    signal leftPressed(int lineIndex, int x, int y)
    signal rightPressed(int lineIndex, int x, int y)
    signal leftReleased(int lineIndex, int x, int y)
    signal rightReleased(int lineIndex, int x, int y)
    signal mouseMoved(int lineIndex, int x, int y)
    property int _index: 0
    property int _patternIndex: 0
    property int _trackIndex: 0
    property double _scrollOffset: 0
    property int _lastTriggeredLine: -1
    readonly property string _tag: "NoteColumnLineContainer"
    property NoteColumnRenderer _renderer
    //! The automation curve a left press landed on, decided at the press: the cursor move a plain
    //! click makes scrolls the curves away from under the mouse before the click arrives.
    property var _pressedCurve: ({})
    function _isCurvePress(): bool {
        return _pressedCurve.id !== undefined;
    }
    function resize(width: int, height: int): void {
        rootItem.width = width;
        rootItem.height = height;
        if (_renderer) {
            _renderer.width = width;
            _renderer.height = height;
        }
    }
    function setLocation(patternIndex: int, trackIndex: int, columnIndex: int): void {
        _patternIndex = patternIndex;
        _trackIndex = trackIndex;
        _index = columnIndex;
        if (!_renderer) {
            _renderer = rendererComponent.createObject(rootItem);
        }
        _renderer.model = noteColumnModelHandler.columnModel(_patternIndex, _trackIndex, _index);
        _scrollOffset = editorService.position.line;
        _lastTriggeredLine = -1;
        _renderer.scrollOffset = _scrollOffset;
    }
    function _updateCurveHover(): void {
        // The curves scroll under a mouse that stays put, and a click does nothing during playback
        if (!_renderer) {
            return;
        }
        if (inputHandler.containsMouse && (!inputHandler.pressed || _isCurvePress()) && !UiService.isPlaying()) {
            _renderer.setHoverPosition(inputHandler.mouseX, inputHandler.mouseY);
        } else {
            _renderer.clearHover();
        }
    }
    function _getEffectiveLine(lineIndex: int): int {
        return lineIndex - editorService.positionBarLine();
    }
    function setPosition(position: var): void {
        if (_scrollOffset !== position.line) {
            _scrollOffset = position.line;
            if (_renderer) {
                _renderer.scrollOffset = _scrollOffset;
                _updateCurveHover();
            }
        }

        if (UiService.isPlaying()) {
            if (_lastTriggeredLine !== position.line) {
                _triggerVolumeMeterAtPosition(position);
                _lastTriggeredLine = position.line;
            }
        } else {
            _lastTriggeredLine = -1;
        }
    }
    function _triggerVolumeMeterAtPosition(position: var): void {
        if (UiService.isPlaying() && mixerService.shouldColumnPlay(_trackIndex, _index)) {
            const velocity = editorService.velocityAtPosition(position.pattern, _trackIndex, _index, position.line);
            if (velocity > 0) {
                const level = mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127;
                volumeMeter.trigger(level);
                _flash(level);
            }
        }
    }
    function _flash(level: double): void {
        if (themeService.columnFlashEnabled && level > 0) {
            flashAnimation.stop();
            flash.opacity = Math.min(level, 1.0) * Constants.columnFlashMaxOpacity;
            flashAnimation.start();
        }
    }
    Component {
        id: rendererComponent
        NoteColumnRenderer {
            anchors.fill: parent
            visibleLines: settingsService.visibleLines
            automationDisplayMode: settingsService.automationDisplayMode
            automationCurveThicknessTenths: settingsService.automationCurveThicknessTenths
            cursorColor: themeService.cursorColor
            textColor: themeService.noteColumnTextColor
            textColorEmpty: themeService.noteColumnTextColorEmpty
            textColorGhost: themeService.noteColumnTextColorGhost
            automationCurveColors: themeService.automationCurveColors
            automationCurveCenterLineColor: themeService.automationCurveCenterLineColor
        }
    }
    // Washes the column in its track colour on every note it plays, and fades out on its own: no
    // timer runs while the column is quiet
    Rectangle {
        id: flash
        anchors.fill: parent
        color: themeService.trackHeaderTextColors[_trackIndex % themeService.trackHeaderTextColors.length]
        opacity: 0
        visible: opacity > 0
        z: 4
        NumberAnimation {
            id: flashAnimation
            target: flash
            property: "opacity"
            to: 0
            duration: Constants.columnFlashDuration
            easing.type: Easing.OutExpo
        }
    }
    VolumeMeter {
        id: volumeMeter
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - rootItem.parent.y - 2 * Constants.columnHeaderHeight : 0
        z: 5
    }
    MouseArea {
        id: inputHandler
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        cursorShape: _renderer && _renderer.automationCurveHovered ? Qt.PointingHandCursor : Qt.ArrowCursor
        onExited: _updateCurveHover()
        onWheel: event => {
            if (!UiService.isPlaying()) {
                if (event.angleDelta.y > 0) {
                    editorService.requestScroll(-1);
                    event.accepted = true;
                } else if (event.angleDelta.y < 0) {
                    editorService.requestScroll(1);
                    event.accepted = true;
                }
            }
        }
        onClicked: mouse => {
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            if (mouse.button === Qt.LeftButton) {
                // A click on a drawn automation curve opens that automation, rather than going
                // through the context menu for it, and leaves the cursor where it was
                if (_isCurvePress()) {
                    const curve = _pressedCurve;
                    _pressedCurve = {};
                    UiService.requestEditAutomationDialogById(curve.kind, curve.id);
                    return;
                }
                uiLogger.info(_tag, `Column ${rootItem._index} left clicked on line ${effectiveLineIndex}`);
                rootItem.leftClicked(effectiveLineIndex, globalPos.x, globalPos.y);
            } else {
                uiLogger.info(_tag, `Column ${rootItem._index} right clicked on line ${effectiveLineIndex}`);
                rootItem.rightClicked(effectiveLineIndex, globalPos.x, globalPos.y);
            }
        }
        onPressed: mouse => {
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            if (mouse.button === Qt.LeftButton) {
                _pressedCurve = _renderer && !UiService.isPlaying() ? _renderer.automationCurveAt(mouse.x, mouse.y) : ({});
                if (_isCurvePress()) {
                    return; // Not the start of a selection either
                }
                uiLogger.info(_tag, `Column ${rootItem._index} left pressed on line ${effectiveLineIndex}`);
                rootItem.leftPressed(effectiveLineIndex, globalPos.x, globalPos.y);
            } else {
                uiLogger.info(_tag, `Column ${rootItem._index} right pressed on line ${effectiveLineIndex}`);
                rootItem.rightPressed(effectiveLineIndex, globalPos.x, globalPos.y);
            }
        }
        onReleased: mouse => {
            _updateCurveHover();
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            if (mouse.button === Qt.LeftButton) {
                if (_isCurvePress()) {
                    return;
                }
                uiLogger.info(_tag, `Column ${rootItem._index} left released on line ${effectiveLineIndex}`);
                rootItem.leftReleased(effectiveLineIndex, globalPos.x, globalPos.y);
            } else {
                uiLogger.info(_tag, `Column ${rootItem._index} right released on line ${effectiveLineIndex}`);
                rootItem.rightReleased(effectiveLineIndex, globalPos.x, globalPos.y);
            }
        }
        onPositionChanged: mouse => {
            _updateCurveHover();
            if (!pressed || _isCurvePress()) {
                return;
            }
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            uiLogger.info(_tag, `Column ${rootItem._index} mouse moved on line ${effectiveLineIndex}`);
            rootItem.mouseMoved(effectiveLineIndex, globalPos.x, globalPos.y);
        }
    }
}
