import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: "transparent"
    border.width: Constants.positionBarBorderWidth
    border.color: themeService.positionBarBorderColor
    Rectangle {
        anchors.fill: parent
        color: themeService.accentColor
        opacity: Constants.positionBarOpacity
    }
}
