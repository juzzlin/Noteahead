import QtQuick 2.15

Rectangle {
    id: rootItem
    color: model.color
    border.color: "#222222"
    border.width: model.border
    opacity: !model.isVirtualRow
    Text {
        id: textElement
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        anchors.centerIn: parent
        text: note ? `${model.note} ${model.velocity.padStart(3, "-")}` : ""
        color: note && note !== "---" ? "#ffffff" : "#888888"
    }
    Loader {
        id: cursorLoader
        anchors.verticalCenter: parent.verticalCenter
        active: model.isFocused
        visible: active
        sourceComponent: cursorComponent
        property alias textElement: textElement
        property int lineColumn: model.lineColumn
    }
    Component {
        id: cursorComponent
        Rectangle {
            property int lineColumn: cursorLoader.lineColumn
            property Item textElement: cursorLoader.textElement
            width: lineColumn === 0 ? 3 * (textElement ? textElement.contentWidth : 0) / 7 : (textElement ? textElement.contentWidth : 0) / 7
            height: textElement ? textElement.contentHeight : 0
            x: lineColumn === 0 ? (textElement ? textElement.x : 0) : (textElement ? textElement.x + (3 + lineColumn) * textElement.contentWidth / 7 : 0)
            color: "red"
            opacity: 0.5
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            clickOnDelegate(mouse, index);
        }
    }
}
