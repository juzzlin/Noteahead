import QtQuick 2.15
import QtQuick.Controls.Universal 2.15
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
    readonly property string _tag: "NoteColumnLineContainer"
    property ListView _listView
    function setLocation(patternIndex: int, trackIndex: int, columnIndex: int): void {
        _patternIndex = patternIndex;
        _trackIndex = trackIndex;
        _index = columnIndex;
        _listView = listViewComponent.createObject(rootItem);
        _listView.model = noteColumnModelHandler.columnModel(_patternIndex, _trackIndex, _index);
        _scrollOffset = editorService.position.line;
        _scrollLines();
        delayedScrollTimer.start(); // Hack for Qt < 6.5
    }
    function _getEffectiveLine(lineIndex: int): int {
        return lineIndex - editorService.positionBarLine();
    }
    function _getGlobalX(delegate: var, mouse: var): int {
        return delegate.mapToItem(delegate.Window.contentItem, Qt.point(mouse.x, mouse.y)).x;
    }
    function _getGlobalY(delegate: var, mouse: var): int {
        return delegate.mapToItem(delegate.Window.contentItem, Qt.point(mouse.x, mouse.y)).y;
    }
    function handleClickOnDelegate(listItemIndex: int, delegate: var, mouse: var): void {
        const effectiveLineIndex = _getEffectiveLine(listItemIndex);
        if (mouse.button === Qt.LeftButton) {
            uiLogger.info(_tag, `Column ${rootItem._index} left clicked on line ${effectiveLineIndex}`);
            rootItem.leftClicked(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        } else {
            uiLogger.info(_tag, `Column ${rootItem._index} right clicked on line ${effectiveLineIndex}`);
            rootItem.rightClicked(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        }
    }
    function handlePressOnDelegate(listItemIndex: int, delegate: var, mouse: var): void {
        const effectiveLineIndex = _getEffectiveLine(listItemIndex);
        if (mouse.button === Qt.LeftButton) {
            uiLogger.info(_tag, `Column ${rootItem._index} left pressed on line ${effectiveLineIndex}`);
            rootItem.leftPressed(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        } else {
            uiLogger.info(_tag, `Column ${rootItem._index} right pressed on line ${effectiveLineIndex}`);
            rootItem.rightPressed(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        }
    }
    function handleReleaseOnDelegate(listItemIndex: int, delegate: var, mouse: var): void {
        const effectiveLineIndex = _getEffectiveLine(listItemIndex);
        if (mouse.button === Qt.LeftButton) {
            uiLogger.info(_tag, `Column ${rootItem._index} left released on line ${effectiveLineIndex}`);
            rootItem.leftReleased(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        } else {
            uiLogger.info(_tag, `Column ${rootItem._index} right released on line ${effectiveLineIndex}`);
            rootItem.rightReleased(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
        }
    }
    function handleMouseMoveOnDelegate(listItemIndex: int, delegate: var, mouse: var): void {
        const offset = Math.floor(mouse.y / delegate.height);
        const effectiveLineIndex = _getEffectiveLine(listItemIndex) + offset;
        uiLogger.info(_tag, `Column ${rootItem._index} mouse moved on line ${effectiveLineIndex}`);
        rootItem.mouseMoved(effectiveLineIndex, _getGlobalX(delegate, mouse), _getGlobalY(delegate, mouse));
    }
    function setPosition(position: var): void {
        _scrollOffset = position.line;
        _scrollLines();
        _triggerVolumeMeterAtPosition(position);
    }
    function _scrollLines(): void {
        if (_listView) {
            _listView.positionViewAtIndex(_scrollOffset, ListView.Beginning);
        }
    }
    function _triggerVolumeMeterAtPosition(position: var): void {
        if (UiService.isPlaying() && mixerService.shouldColumnPlay(_trackIndex, _index)) {
            const velocity = editorService.velocityAtPosition(position.pattern, _trackIndex, _index, position.line);
            volumeMeter.trigger(mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127);
        }
    }
    Component {
        id: listViewComponent
        ListView {
            id: listView
            anchors.fill: parent
            cacheBuffer: 2
            clip: true
            delegate: NoteColumn_LineDelegate {
                height: rootItem.height / settingsService.visibleLines
                width: listView.width
            }
            interactive: false
        }
    }
    Timer {
        id: delayedScrollTimer
        interval: 1
        repeat: false
        onTriggered: _scrollLines()
    }
    VolumeMeter {
        id: volumeMeter
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - rootItem.parent.y - 2 * columnHeader.height : 0
        z: 5
    }
    MouseArea {
        id: wheelHandler
        anchors.fill: parent
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
    }
}
