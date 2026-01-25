import QtQuick 2.15

Rectangle {
    id: rootItem
    color: model.color
    border.color: "#222222"
    border.width: 0
    opacity: !model.isVirtualRow
    Text {
        id: textElement
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.centerIn: parent
        text: model.line
        color: note && note !== "---" ? "#ffffff" : "#888888"
    }
    Rectangle {
        width: {
            if (model.lineColumn === 0) return 3 * (textElement ? textElement.contentWidth : 0) / 10;
            return (textElement ? textElement.contentWidth : 0) / 10;
        }
        height: textElement.contentHeight
        x: {
            if (model.lineColumn === 0) return (textElement ? textElement.x : 0);
            if (model.lineColumn <= 3) return (textElement ? textElement.x + (3 + model.lineColumn) * textElement.contentWidth / 10 : 0);
            return (textElement ? textElement.x + (4 + model.lineColumn) * textElement.contentWidth / 10 : 0);
        }
        color: "red"
        opacity: 0.5
        visible: model.isFocused
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            handleClickOnDelegate(index, rootItem, mouse);
        }
        onPressed: mouse => {
            handlePressOnDelegate(index, rootItem, mouse);
        }
        onReleased: mouse => {
            handleReleaseOnDelegate(index, rootItem, mouse);
        }
        onPositionChanged: mouse => {
            handleMouseMoveOnDelegate(index, rootItem, mouse);
        }
    }
}
