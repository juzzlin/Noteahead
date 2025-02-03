import QtQuick 2.15
import QtQuick.Layouts 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnCellBackgroundColor
    border.color: Constants.noteColumnCellBorderColor
    border.width: 1
    property string note: ""
    property string velocity: ""
    property int index: 0
    property bool _focused: false
    property var _indexHighlight
    property int _lineColumnIndex: 0
    readonly property string _fontFamily: "monospace"
    readonly property color _textColor: _isValidNote(note) ? Constants.noteColumnTextColor : Constants.noteColumnTextColorEmpty
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
        velocityCell.setFocused(focused, lineColumnIndex);
    }
    function _isValidNote(note) {
        return note && note !== editorService.noDataString();
    }
    Text {
        id: noteText
        text: note
        font.pixelSize: parent.height * 0.8
        font.family: _fontFamily
        color: _textColor
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: width / 3
        Cursor {
            id: cursor
            visible: rootItem._focused && _lineColumnIndex === 0
        }
    }
    VelocityCell {
        id: velocityCell
        velocity: rootItem.velocity
        isValid: _isValidNote(note)
        height: parent.height
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.horizontalCenter
    }
    Component {
        id: indexHighlightComponent
        Rectangle {
            anchors.fill: parent
            color: "white"
            visible: opacity > 0
        }
    }
    function _indexHighlightOpacity(linesPerBeat) {
        const _beatLine1 = linesPerBeat;
        const _beatLine2 = _beatLine1 % 3 ? _beatLine1 / 2 : _beatLine1 / 3;
        const _beatLine3 = _beatLine1 % 6 ? _beatLine1 / 4 : _beatLine1 / 6;
        if (!(index % _beatLine1))
            return 0.25;
        if (!(index % _beatLine3) && !(index % _beatLine2))
            return 0.10;
        if (!(index % _beatLine3))
            return 0.05;
        return 0;
    }
    function _updateIndexHighlight() {
        const indexHighlightOpacity = _indexHighlightOpacity(editorService.linesPerBeat);
        if (indexHighlightOpacity > 0) {
            if (!_indexHighlight) {
                _indexHighlight = indexHighlightComponent.createObject(rootItem);
            }
            _indexHighlight.opacity = indexHighlightOpacity;
        } else {
            if (_indexHighlight) {
                _indexHighlight.destroy();
                _indexHighlight = null;
            }
        }
    }
    Component.onCompleted: {
        editorService.linesPerBeatChanged.connect(_updateIndexHighlight);
        _updateIndexHighlight();
    }
}
