import QtQuick 2.15

Rectangle {
    id: rootItem
    color: "#000000"
    border.color: "#222222"
    border.width: 1
    function setNoteData(note: int, velocity: int): void {
        noteVelocityText.text = `${note} ${velocity.padStart(3, "-")}`;
        noteVelocityText.color = note && note !== "---" ? "#ffffff" : "#888888";
    }
    function setCursor(lineCursor: var, columnIndex: int): void {
        lineCursor.parent = rootItem;
        lineCursor.width = columnIndex === 0 ? 3 * noteVelocityText.contentWidth / 7 : noteVelocityText.contentWidth / 7;
        lineCursor.height = noteVelocityText.contentHeight;
        lineCursor.x = columnIndex === 0 ? noteVelocityText.x : noteVelocityText.x + (3 + columnIndex) * noteVelocityText.contentWidth / 7;
    }
    Text {
        id: noteVelocityText
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
