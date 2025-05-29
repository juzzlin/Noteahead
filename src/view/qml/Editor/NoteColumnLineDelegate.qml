import QtQuick 2.15

Rectangle {
    id: rootItem
    color: "#000000"
    border.color: "#222222"
    border.width: 1
    opacity: !model.padding
    Text {
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        text: note ? `${model.note} ${model.velocity.padStart(3, "-")}` : ""
        color: note && note !== "---" ? "#ffffff" : "#888888"
    }
}
