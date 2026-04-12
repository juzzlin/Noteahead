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
    function _getEffectiveLine(lineIndex: int): int {
        return lineIndex - editorService.positionBarLine();
    }
    function setPosition(position: var): void {
        if (_scrollOffset !== position.line) {
            _scrollOffset = position.line;
            if (_renderer) {
                _renderer.scrollOffset = _scrollOffset;
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
                volumeMeter.trigger(mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127);
            }
        }
    }
    Component {
        id: rendererComponent
        NoteColumnRenderer {
            anchors.fill: parent
            visibleLines: settingsService.visibleLines
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
                uiLogger.info(_tag, `Column ${rootItem._index} left pressed on line ${effectiveLineIndex}`);
                rootItem.leftPressed(effectiveLineIndex, globalPos.x, globalPos.y);
            } else {
                uiLogger.info(_tag, `Column ${rootItem._index} right pressed on line ${effectiveLineIndex}`);
                rootItem.rightPressed(effectiveLineIndex, globalPos.x, globalPos.y);
            }
        }
        onReleased: mouse => {
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            if (mouse.button === Qt.LeftButton) {
                uiLogger.info(_tag, `Column ${rootItem._index} left released on line ${effectiveLineIndex}`);
                rootItem.leftReleased(effectiveLineIndex, globalPos.x, globalPos.y);
            } else {
                uiLogger.info(_tag, `Column ${rootItem._index} right released on line ${effectiveLineIndex}`);
                rootItem.rightReleased(effectiveLineIndex, globalPos.x, globalPos.y);
            }
        }
        onPositionChanged: mouse => {
            const rowHeight = height / settingsService.visibleLines;
            const rowIndex = Math.floor(mouse.y / rowHeight) + Math.floor(_scrollOffset);
            const effectiveLineIndex = _getEffectiveLine(rowIndex);
            const globalPos = mapToItem(Window.contentItem, Qt.point(mouse.x, mouse.y));
            uiLogger.info(_tag, `Column ${rootItem._index} mouse moved on line ${effectiveLineIndex}`);
            rootItem.mouseMoved(effectiveLineIndex, globalPos.x, globalPos.y);
        }
    }
}
