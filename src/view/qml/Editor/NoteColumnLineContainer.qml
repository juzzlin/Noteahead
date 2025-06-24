import QtQuick 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnBackgroundColor
    clip: true
    signal leftClicked(int x, int y, int lineIndex)
    signal rightClicked(int x, int y, int lineIndex)
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
        _listView.positionViewAtIndex(0, ListView.Beginning);
        delayedPositioningTimer.start(); // Hack for Qt < 6.5
    }
    function clickOnDelegate(mouse: var, lineIndex: int): void {
        if (mouse.button === Qt.LeftButton) {
            uiLogger.info(_tag, `Column ${rootItem._index} left clicked on line ${lineIndex}`);
            rootItem.leftClicked(mouse.x, mouse.y, lineIndex - editorService.positionBarLine());
        } else {
            uiLogger.info(_tag, `Column ${rootItem._index} right clicked on line ${lineIndex}`);
            rootItem.rightClicked(mouse.x, mouse.y, lineIndex - editorService.positionBarLine());
        }
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
            delegate: NoteColumnLineDelegate {
                height: rootItem.height / settingsService.visibleLines
                width: listView.width
            }
            interactive: false
        }
    }
    Timer {
        id: delayedPositioningTimer
        interval: 1
        repeat: false
        onTriggered: {
            if (_listView) {
                _listView.positionViewAtIndex(0, ListView.Beginning);
            }
        }
    }
    Rectangle {
        id: borderRectangle
        color: "transparent"
        border.color: Constants.noteColumnBorderColor
        border.width: 1
        anchors.fill: parent
        z: 2
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
