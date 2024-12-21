import QtQuick 2.15
import ".."

Item {
    id: rootItem
    width: velocityDigit0.width + velocityDigit1.width + velocityDigit2.width
    property bool isValid: false
    property string velocity: ""
    property bool _focused: false
    property int _lineColumnIndex: 0
    readonly property string _fontFamily: "monospace"
    readonly property color _textColor: isValid ? Constants.noteColumnTextColor : Constants.noteColumnTextColorEmpty
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
    }
    function _getVelocityDigit(digit) {
        const index = velocity.length - 1 - digit;
        return 0 >= index < velocity.length ? velocity[index] : editorService.noDataString()[0];
    }
    Row {
        anchors.centerIn: parent
        height: parent.height
        Text {
            id: velocityDigit2
            text: _getVelocityDigit(2)
            font.pixelSize: rootItem.height * 0.8
            font.family: _fontFamily
            color: _textColor
            anchors.verticalCenter: parent.verticalCenter
            Cursor {
                visible: rootItem._focused && _lineColumnIndex === 1
            }
        }
        Text {
            id: velocityDigit1
            text: _getVelocityDigit(1)
            font.pixelSize: velocityDigit2.font.pixelSize
            font.family: velocityDigit2.font.family
            color: velocityDigit2.color
            anchors.verticalCenter: parent.verticalCenter
            Cursor {
                visible: rootItem._focused && _lineColumnIndex === 2
            }
        }
        Text {
            id: velocityDigit0
            text: _getVelocityDigit(0)
            font.pixelSize: velocityDigit2.font.pixelSize
            font.family: velocityDigit2.font.family
            color: velocityDigit2.color
            anchors.verticalCenter: parent.verticalCenter
            Cursor {
                visible: rootItem._focused && _lineColumnIndex === 3
            }
        }
    }
}
