import QtQuick 2.15

Rectangle {
    id: rootItem
    color: model.color
    border.color: "#222222"
    border.width: model.border
    opacity: !model.isVirtualRow
    function setCursor(lineCursor: var, columnIndex: int): void {
        lineCursor.parent = rootItem;
        lineCursor.width = columnIndex === 0 ? 3 * text.contentWidth / 7 : text.contentWidth / 7;
        lineCursor.height = text.contentHeight;
        lineCursor.x = columnIndex === 0 ? text.x : text.x + (3 + columnIndex) * text.contentWidth / 7;
    }
    Text {
        id: text
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        text: note ? `${model.note} ${model.velocity.padStart(3, "-")}` : ""
        color: note && note !== "---" ? "#ffffff" : "#888888"
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            clickOnDelegate(mouse, index);
        }
    }
}
