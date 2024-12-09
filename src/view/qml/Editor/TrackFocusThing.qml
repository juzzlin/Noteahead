import QtQuick 2.15
import ".."

Item {
    id: trackHighlight
    property alias borderColor: topLeft.color // Color of the highlights
    property alias cornerSize: topLeft.width // Thickness of the highlights
    property alias cornerLength: topLeft.height // Length of the L-shapes
    anchors.fill: parent
    // Top-left corner
    Rectangle {
        id: topLeft
        color: Constants.trackBorderFocusedColor
        width: Constants.trackBorderFocusedWidth
        height: parent.width * 0.125
        radius: 4
        anchors.left: parent.left
        anchors.top: parent.top
    }
    Rectangle {
        color: topLeft.color
        width: topLeft.height
        height: topLeft.width
        radius: topLeft.radius
        anchors.left: parent.left
        anchors.top: parent.top
    }
    // Top-right corner
    Rectangle {
        id: topRight
        color: topLeft.color
        width: topLeft.height
        height: topLeft.width
        radius: topLeft.radius
        anchors.right: parent.right
        anchors.top: parent.top
    }
    Rectangle {
        color: topLeft.color
        width: topLeft.width
        height: topLeft.height
        radius: topLeft.radius
        anchors.right: parent.right
        anchors.top: parent.top
    }
    // Bottom-left corner
    Rectangle {
        id: bottomLeft
        color: topLeft.color
        width: topLeft.width
        height: topLeft.height
        radius: topLeft.radius
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }
    Rectangle {
        color: topLeft.color
        width: topLeft.height
        height: topLeft.width
        radius: topLeft.radius
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }
    // Bottom-right corner
    Rectangle {
        id: bottomRight
        color: topLeft.color
        width: topLeft.height
        height: topLeft.width
        radius: topLeft.radius
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
    Rectangle {
        color: topLeft.color
        width: topLeft.width
        height: topLeft.height
        radius: topLeft.radius
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
