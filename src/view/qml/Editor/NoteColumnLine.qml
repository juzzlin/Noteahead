import QtQuick 2.15

Rectangle {
    id: rootItem
    color: _scaledColor(_indexHighlightOpacity(linesPerBeat))
    border.color: "#222222"
    border.width: 1
    property int index: 0
    property bool _focused: false
    property var _indexHighlight
    property int _lineColumnIndex: 0
    property int linesPerBeat: 4 // Default value
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    function setNoteData(note, velocity) {
        noteVelocityText.text = `${note} ${velocity.padStart(3, "-")}`;
    }
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
    }
    function updateIndexHighlight(linesPerBeat) {
        rootItem.color = _scaledColor(_indexHighlightOpacity(linesPerBeat));
    }
    Text {
        id: noteVelocityText
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        color: rootItem._isValidNote(noteVelocityText.text.split(" ")[0]) ? "#ffffff" : "#888888"
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Rectangle {
        id: combinedCursor
        visible: rootItem._focused
        width: _lineColumnIndex === 0 ? 3 * noteVelocityText.contentWidth / 7 : noteVelocityText.contentWidth / 7
        height: noteVelocityText.contentHeight
        color: "red"
        opacity: 0.5
        anchors.verticalCenter: parent.verticalCenter
        x: _lineColumnIndex === 0 ? noteVelocityText.x : noteVelocityText.x + (3 + _lineColumnIndex) * noteVelocityText.contentWidth / 7
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
    function _isValidNote(note) {
        return note && note !== editorService.noDataString();
    }
    function _scaledColor(opacity) {
        const value = Math.round(255 * opacity);
        return "#" + value.toString(16).padStart(2, "0").repeat(3);
    }
}
