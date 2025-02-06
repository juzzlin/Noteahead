import QtQuick 2.15

Rectangle {
    id: rootItem
    color: "#000000"
    border.color: "#222222"
    border.width: 1
    property int index: 0
    property bool _focused: false
    property var _indexHighlight
    property int _lineColumnIndex: 0
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    function setNoteData(note, velocity) {
        noteText.text = note;
        velocityText.text = velocity.padStart(3, "-");
    }
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
    }
    function _isValidNote(note) {
        return note && note !== editorService.noDataString();
    }
    Text {
        id: noteText
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        color: _isValidNote(text) ? "#ffffff" : "#888888"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: width / 3
    }
    Text {
        id: velocityText
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        color: rootItem._isValidNote(noteText.text) ? "#ffffff" : "#888888"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.horizontalCenter
    }
    Rectangle {
        id: combinedCursor
        visible: rootItem._focused
        width: _lineColumnIndex === 0 ? noteText.contentWidth : velocityText.contentWidth / 3
        height: noteText.contentHeight
        color: "red"
        opacity: 0.5
        anchors.verticalCenter: parent.verticalCenter
        x: _lineColumnIndex === 0 ? noteText.x : velocityText.x + (_lineColumnIndex - 1) * (velocityText.contentWidth / 3)
    }
    Component {
        id: indexHighlightComponent
        Rectangle {
            anchors.fill: parent
            color: "#ffffff"
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
    function updateIndexHighlight(linesPerBeat) {
        const indexHighlightOpacity = _indexHighlightOpacity(linesPerBeat);
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
}
