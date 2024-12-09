import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: "transparent"
    border.width: Constants.positionBarBorderWidth
    border.color: Constants.positionBarBorderColor
    Rectangle {
        anchors.fill: parent
        color: Constants.positionBarColor
        opacity: Constants.positionBarOpacity
    }
}
