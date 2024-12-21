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
    property int _lineColumnIndex: 0
    readonly property string _fontFamily: "monospace"
    readonly property color _textColor: _isValidNote(note) ? Constants.noteColumnTextColor : Constants.noteColumnTextColorEmpty
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
        velocityCell.setFocused(focused, lineColumnIndex);
    }
    function _isValidNote(note) {
        return note && note !== editorService.noDataString();
    }
    Row {
        anchors.centerIn: parent
        height: parent.height
        Text {
            id: noteText
            text: note
            font.pixelSize: parent.height * 0.8
            font.family: _fontFamily
            color: _textColor
            anchors.verticalCenter: parent.verticalCenter
            Cursor {
                id: cursor
                visible: rootItem._focused && _lineColumnIndex === 0
            }
        }
        Text {
            id: spacer
            text: " "
            font.pixelSize: noteText.font.pixelSize
            font.family: noteText.font.family
            anchors.verticalCenter: parent.verticalCenter
        }
        VelocityCell {
            id: velocityCell
            velocity: rootItem.velocity
            isValid: _isValidNote(note)
            height: parent.height
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    IndexHighlight {
        anchors.fill: parent
        index: parent.index
    }
}
