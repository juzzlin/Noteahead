import QtQuick 2.15

Item {
    id: rootItem
    width: velocityDigit0.width + velocityDigit1.width + velocityDigit2.width
    height: velocityDigit2.height
    property bool isValid: false
    property string velocity: ""
    property bool _focused: false
    property int _lineColumnIndex: 0
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
    }
    function _getVelocityDigit(digit) {
        const index = velocity.length - 1 - digit;
        return 0 <= index && index < velocity.length ? velocity[index] : editorService.noDataString()[0];
    }
    Text {
        id: velocityDigit2
        text: _getVelocityDigit(2)
        font.pixelSize: rootItem.height * 0.8
        font.family: "monospace"
        color: isValid ? "#ffffff" : "#888888"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
    }
    Text {
        id: velocityDigit1
        text: _getVelocityDigit(1)
        font.pixelSize: velocityDigit2.font.pixelSize
        font.family: velocityDigit2.font.family
        color: velocityDigit2.color
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: velocityDigit2.right  // Anchored to the right of velocityDigit2
    }
    Text {
        id: velocityDigit0
        text: _getVelocityDigit(0)
        font.pixelSize: velocityDigit2.font.pixelSize
        font.family: velocityDigit2.font.family
        color: velocityDigit2.color
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: velocityDigit1.right  // Anchored to the right of velocityDigit1
    }
    Cursor {
        visible: rootItem._focused && (_lineColumnIndex === 1 || _lineColumnIndex === 2 || _lineColumnIndex === 3)
        x: _lineColumnIndex === 1 ? velocityDigit2.x : _lineColumnIndex === 2 ? velocityDigit1.x : velocityDigit0.x
        y: velocityDigit2.y
        width: _lineColumnIndex === 1 ? velocityDigit2.width : _lineColumnIndex === 2 ? velocityDigit1.width : velocityDigit0.width
        height: velocityDigit2.height
    }
}
