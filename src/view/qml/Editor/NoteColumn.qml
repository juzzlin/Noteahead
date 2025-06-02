import QtQuick 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    signal leftClicked(int x, int y, int lineIndex)
    signal rightClicked(int x, int y, int lineIndex)
    property int _index: 0
    property int _patternIndex: 0
    property int _trackIndex: 0
    property Item _positionBar
    property bool _dataUpdated: false
    readonly property string _tag: "NoteColumn"
    function resize(width: int, height: int): void {
        rootItem.width = width;
        rootItem.height = height;
        lineContainer.width = width;
        lineContainer.height = height - columnHeader.height;
    }
    function dataUpdated(): bool {
        return _dataUpdated;
    }
    function index(): int {
        return _index;
    }
    function setLocation(patternIndex: int, trackIndex: int, columnIndex: int): void {
        _patternIndex = patternIndex;
        _trackIndex = trackIndex;
        _index = columnIndex;
        columnHeader.setIndex(_trackIndex); // Uses some color as the parent track
        lineContainer.setLocation(patternIndex, trackIndex, columnIndex);
    }
    function setName(name: string): void {
        columnHeader.setName(name);
    }
    function setMuted(muted: bool): void {
        columnHeader.setMuted(muted);
    }
    function setSoloed(soloed: bool): void {
        columnHeader.setSoloed(soloed);
    }
    function setVelocityScale(value: int): void {
        columnHeader.setVelocityScale(value);
    }
    function setPosition(position: var): void {
        lineContainer.setPosition(position);
    }
    function setPositionBar(positionBar: var): void {
        _positionBar = positionBar;
    }
    NoteColumnHeader {
        id: columnHeader
        anchors.top: parent.top
        width: parent.width
    }
    NoteColumnLineContainer {
        id: lineContainer
        anchors.top: columnHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
    Component.onCompleted: {
        columnHeader.invertedMuteRequested.connect(() => mixerService.invertMutedColumns(_trackIndex, _index));
        columnHeader.invertedSoloRequested.connect(() => mixerService.invertSoloedColumns(_trackIndex, _index));
        columnHeader.muteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, true));
        columnHeader.soloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, true));
        columnHeader.unmuteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, false));
        columnHeader.unsoloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, false));
        columnHeader.nameChanged.connect(name => editorService.setColumnName(_trackIndex, _index, name));
        columnHeader.velocityScaleRequested.connect(() => UiService.requestColumnVelocityScaleDialog(_trackIndex, _index));
        lineContainer.leftClicked.connect(leftClicked);
        lineContainer.rightClicked.connect(rightClicked);
    }
    Component.onDestruction: {
        lineContainer.leftClicked.disconnect(leftClicked);
        lineContainer.rightClicked.disconnect(rightClicked);
    }
}
