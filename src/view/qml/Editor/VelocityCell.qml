import QtQuick 2.15

Item {
    id: rootItem
    width: velocityText.contentWidth
    height: velocityText.contentHeight
    property bool isValid: false
    property bool _focused: false
    property int _lineColumnIndex: 0
    function setFocused(focused, lineColumnIndex) {
        _focused = focused;
        _lineColumnIndex = lineColumnIndex;
    }
    function setVelocity(velocity) {
        velocityText.text = velocity.padStart(3, "-"); // Ensures we always have three characters
    }
    Text {
        id: velocityText
        font.pixelSize: rootItem.height * 0.8
        font.family: "monospace"
        color: isValid ? "#ffffff" : "#888888"
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        id: cursor
        visible: rootItem._focused && (_lineColumnIndex >= 1 && _lineColumnIndex <= 3)
        width: velocityText.contentWidth / 3
        height: velocityText.contentHeight
        color: "red"
        opacity: 0.5
        anchors.verticalCenter: velocityText.verticalCenter
        x: (_lineColumnIndex - 1) * (velocityText.contentWidth / 3)
    }
}
